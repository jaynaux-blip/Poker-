"""SM_Card and the deck's faces: the Spin Cycle Club's plastic poker cards.

Poker-size (63.5 x 88.9 mm) jumbo-index cards, like card rooms use: big corner indices you can read
by lifting just the near edge. Each face (and the back) is rendered to unreal/Art/Cards/T_Card_<code>.png
at 1024 x 1024 with the card centered (u from CARD_U0 to CARD_U1); Unreal masks the rounded corners.

SM_Card is a flat grid, not baked: the game bends it in the material (a peek curls the near edge), so
its faces are subdivided. Material slots: 0 face, 1 back, 2 edge. Coordinates (meters): origin at the
card's center, face up (+Z), the top of the face toward +Y.
"""
import math
import os
import shutil

import bmesh

from artkit import core
from artkit.sheet import Sheet

MESHES = ['SM_Card']
NO_BAKE = True

W, H, R_CORNER, THICK = 0.0635, 0.0889, 0.0032, 0.0003
CARD_U0 = (H - W) / 2 / H
CARD_U1 = 1.0 - CARD_U0
CARDS_DIR = os.path.join(core.ROOT, 'unreal', 'Art', 'Cards')
RANKS = ['2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A']
SUITS = ['c', 'd', 'h', 's']
RED, BLACK = 0xc01a28, 0x151516
PAPER = 0xf6f3ec
REVIEW_VIEWS = [('front', -20, 50, 2.0), ('top', 0, 88, 1.6), ('detail', -30, 40, 0.8)]


# ------------------------------------------------------------------ suit shapes (size = pip height, mm)

def pip(s, suit, cx, cy, size, color, flip=False):
    """One suit symbol centered at (cx, cy); flip turns it upside down (the lower half of the layout)."""
    k = -1.0 if flip else 1.0

    def p(x, y):
        return (cx + k * x * size, cy + k * y * size)

    if suit == 'd':
        s.poly([p(0, 0.5), p(0.36, 0), p(0, -0.5), p(-0.36, 0)], color, rough=0.3)
    elif suit == 'h':
        s.circle(*p(-0.235, 0.2), 0.265 * size, color, n=64, rough=0.3)
        s.circle(*p(0.235, 0.2), 0.265 * size, color, n=64, rough=0.3)
        s.poly([p(-0.49, 0.13), p(0.49, 0.13), p(0.0, -0.5)], color, rough=0.3)
    elif suit == 's':
        s.circle(*p(-0.235, -0.12), 0.25 * size, color, n=64, rough=0.3)
        s.circle(*p(0.235, -0.12), 0.25 * size, color, n=64, rough=0.3)
        s.poly([p(0.0, 0.5), p(-0.47, -0.08), p(0.47, -0.08)], color, rough=0.3)
        s.poly([p(0.0, -0.1), p(0.15, -0.5), p(-0.15, -0.5)], color, rough=0.3)
    else:  # clubs
        s.circle(*p(0.0, 0.24), 0.225 * size, color, n=64, rough=0.3)
        s.circle(*p(-0.245, -0.07), 0.225 * size, color, n=64, rough=0.3)
        s.circle(*p(0.245, -0.07), 0.225 * size, color, n=64, rough=0.3)
        s.poly([p(0.0, 0.1), p(0.15, -0.5), p(-0.15, -0.5)], color, rough=0.3)


# Pip layouts for 2-10 in a 1 x 1 box (x, y from the center; y > 0 is the top half, drawn upright).
LAYOUTS = {
    2: [(0, 0.5), (0, -0.5)],
    3: [(0, 0.5), (0, 0), (0, -0.5)],
    4: [(-1, 0.5), (1, 0.5), (-1, -0.5), (1, -0.5)],
    5: [(-1, 0.5), (1, 0.5), (0, 0), (-1, -0.5), (1, -0.5)],
    6: [(-1, 0.5), (1, 0.5), (-1, 0), (1, 0), (-1, -0.5), (1, -0.5)],
    7: [(-1, 0.5), (1, 0.5), (0, 0.25), (-1, 0), (1, 0), (-1, -0.5), (1, -0.5)],
    8: [(-1, 0.5), (1, 0.5), (0, 0.25), (-1, 0), (1, 0), (0, -0.25), (-1, -0.5), (1, -0.5)],
    9: [(-1, 0.5), (1, 0.5), (-1, 0.167), (1, 0.167), (0, 0), (-1, -0.167), (1, -0.167), (-1, -0.5), (1, -0.5)],
    10: [(-1, 0.5), (1, 0.5), (0, 0.333), (-1, 0.167), (1, 0.167), (-1, -0.167), (1, -0.167), (0, -0.333), (-1, -0.5), (1, -0.5)],
}


def card_face(rank, suit):
    """One face as a Sheet (millimeters): the card spans x0..x0+63.5 inside an 88.9 mm square."""
    size = H * 1000
    s = Sheet(size, size)
    x0 = (H - W) * 1000 / 2
    w, h = W * 1000, H * 1000
    cx, cy = x0 + w / 2, h / 2
    color = RED if suit in 'dh' else BLACK
    s.poly(core.rounded_rect(x0, 0, x0 + w, h, R_CORNER * 1000, steps=10), PAPER, rough=0.35)
    label = '10' if rank == 'T' else rank

    def corner(flip):
        # Jumbo index: a big rank with its suit under it, top left (and bottom right, upside down).
        def t(x, y):
            return (2 * cx - x, 2 * cy - y) if flip else (x, y)
        tx, ty = t(x0 + (2.4 if label == '10' else 3.4), h - 20.0)
        s.text(label, tx, ty, 21.0 if label != '10' else 18.5, color, face='Bold', rot=math.pi if flip else 0.0,
               tracking=0.82 if label == '10' else 1.0, rough=0.3)
        px, py = t(x0 + 8.6, h - 29.5)
        pip(s, suit, px, py, 9.0, color, flip=flip)

    corner(False)
    corner(True)
    if rank == 'A':
        pip(s, suit, cx, cy, 30.0 if suit != 's' else 36.0, color)
        if suit == 's':
            s.ring(cx, cy - 1.5, 21.0, 21.6, color, rough=0.3)
    elif rank in 'JQK':
        # Court cards: a framed panel with the rank, the suit and a simple emblem in the court colors.
        fx0, fy0, fx1, fy1 = x0 + 14.0, 14.0, x0 + w - 14.0, h - 14.0
        s.poly(core.rounded_rect(fx0, fy0, fx1, fy1, 2.0, steps=6), color, rough=0.3)
        s.poly(core.rounded_rect(fx0 + 0.9, fy0 + 0.9, fx1 - 0.9, fy1 - 0.9, 1.4, steps=6), PAPER, rough=0.3)
        s.rect(fx0 + 0.9, cy - 0.5, fx1 - fx0 - 1.8, 1.0, color, rough=0.3)
        gold, blue = 0xd39c2e, 0x284c96
        for flip in (False, True):
            def t(x, y):
                return (2 * cx - x, 2 * cy - y) if flip else (x, y)
            # Band of court colors across the top of each half.
            by = cy + 22.0
            s.poly([t(fx0 + 1.5, by), t(fx1 - 1.5, by), t(fx1 - 1.5, by + 4.0), t(fx0 + 1.5, by + 4.0)], gold, rough=0.3)
            s.poly([t(fx0 + 1.5, by - 2.0), t(fx1 - 1.5, by - 2.0), t(fx1 - 1.5, by - 0.8), t(fx0 + 1.5, by - 0.8)], blue, rough=0.3)
            # The emblem: a crown (K), a coronet (Q) or a cap feather (J).
            ex, ey = t(cx, cy + 12.5)
            k = -1.0 if flip else 1.0
            if rank == 'K':
                pts = [(-7, -3), (7, -3), (7, 3), (4.7, 0.5), (2.3, 4.5), (0, 0.8), (-2.3, 4.5), (-4.7, 0.5), (-7, 3)]
            elif rank == 'Q':
                pts = [(-6, -3), (6, -3), (6, 1), (3, 3.5), (0, 1.5), (-3, 3.5), (-6, 1)]
            else:
                pts = [(-6, -3), (6, -3), (4, 0), (7, 4), (1, 1.5), (-6, 1)]
            s.poly([(ex + k * x, ey + k * y) for x, y in pts], gold, rough=0.3)
            lx, ly = t(cx, cy + 1.5)
            s.text(label, lx, ly, 13.0, color, face='Black', align='CENTER', rot=math.pi if flip else 0.0, rough=0.3)
            pip(s, suit, *t(cx - 11.0, cy + 7.5), 6.0, color, flip=flip)
            pip(s, suit, *t(cx + 11.0, cy + 7.5), 6.0, color, flip=flip)
    else:
        n = int(label)
        # The pips sit inside the central box, clear of the indices.
        bx, by = 10.8, 28.0
        psize = 10.5 if n <= 3 else 9.6
        for x, y in LAYOUTS[n]:
            pip(s, suit, cx + x * bx, cy + y * by * 2 * 0.98, psize, color, flip=y < 0)
    return s


def card_back():
    """The back: navy with a white border, a fine lattice and the club's spade in an oval."""
    size = H * 1000
    s = Sheet(size, size)
    x0 = (H - W) * 1000 / 2
    w, h = W * 1000, H * 1000
    cx, cy = x0 + w / 2, h / 2
    navy, line = 0x1a2849, 0x3a5286
    s.poly(core.rounded_rect(x0, 0, x0 + w, h, R_CORNER * 1000, steps=10), PAPER, rough=0.35)
    ix0, iy0, ix1, iy1 = x0 + 3.6, 3.6, x0 + w - 3.6, h - 3.6
    s.poly(core.rounded_rect(ix0, iy0, ix1, iy1, 1.6, steps=6), navy, rough=0.35)
    # Diagonal lattice, clipped to the panel by drawing it as short segments.
    step = 3.2
    k = -h
    while k < w + h:
        for sign in (1, -1):
            pts = []
            for t in range(0, 200):
                x = ix0 + k + t * 0.5 * sign
                y = iy0 + t * 0.5
                if ix0 + 0.6 <= x <= ix1 - 0.6 and iy0 + 0.6 <= y <= iy1 - 0.6:
                    pts.append((x, y))
            if len(pts) > 1:
                (xa, ya), (xb, yb) = pts[0], pts[-1]
                dx, dy = xb - xa, yb - ya
                ln = math.hypot(dx, dy)
                nx, ny = -dy / ln * 0.16, dx / ln * 0.16
                s.poly([(xa - nx, ya - ny), (xb - nx, yb - ny), (xb + nx, yb + ny), (xa + nx, ya + ny)], line, rough=0.35)
        k += step
    s.poly([(cx + 13.5 * math.cos(a) * 1.0, cy + 19.0 * math.sin(a)) for a in [2 * math.pi * i / 96 for i in range(96)]], navy, rough=0.35)
    s.ring(cx, cy, 12.4, 13.0, PAPER, rough=0.35, n=128)
    pip(s, 's', cx, cy + 2.0, 12.0, PAPER)
    s.text('SPIN CYCLE', cx, cy - 11.0, 2.6, PAPER, face='Bold', align='CENTER', tracking=1.3, rough=0.35)
    return s


# ------------------------------------------------------------------ mesh

def grid_face(bm, uv, z, flip, nx=30, ny=42):
    """A subdivided rectangle over the card at height z; UVs map the card into the 1024 square.

    About 2 mm cells: the peek curls the card around a 16 mm radius, and a coarser grid shows the bend as
    creases (the face looks crumpled)."""
    verts = [[bm.verts.new((-W / 2 + W * i / nx, -H / 2 + H * j / ny, z)) for i in range(nx + 1)] for j in range(ny + 1)]
    for j in range(ny):
        for i in range(nx):
            quad = [verts[j][i], verts[j][i + 1], verts[j + 1][i + 1], verts[j + 1][i]]
            f = bm.faces.new(list(reversed(quad)) if flip else quad)
            for loop in f.loops:
                x, y = loop.vert.co.x, loop.vert.co.y
                u = CARD_U0 + (x / W + 0.5) * (CARD_U1 - CARD_U0)
                # The back is seen from below: mirror it so its print reads correctly.
                loop[uv].uv = ((1.0 - u) if flip else u, y / H + 0.5)
    return verts


def build():
    core.reset()
    os.makedirs(CARDS_DIR, exist_ok=True)
    # SHORTSTACK_CARD_MESH_ONLY=1 rebuilds just the mesh, keeping the rendered faces.
    if not os.environ.get('SHORTSTACK_CARD_MESH_ONLY'):
        renders = [(f'{r}{s}', card_face(r, s)) for s in SUITS for r in RANKS] + [('back', card_back())]
        for code, sheet in renders:
            color, _ = sheet.render(f'card_{code}', 1024, samples=16)
            shutil.copyfile(color, os.path.join(CARDS_DIR, f'T_Card_{code}.png'))
        print(f'[cards] rendered {len(renders)} faces into {os.path.relpath(CARDS_DIR, core.ROOT)}')

    core.reset()
    mats = []
    for name, hexc in (('card_face', PAPER), ('card_back', 0x1a2849), ('card_edge', 0xe9e6de)):
        m = core.Mat(name)
        m.set('Base Color', core.hex_linear(hexc))
        m.set('Roughness', 0.35)
        mats.append(m)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    top = grid_face(bm, uv, THICK / 2, False)
    bottom = grid_face(bm, uv, -THICK / 2, True)
    n_top = len(bm.faces) // 2
    # The edge all around (square corners: the material masks the rounded ones on both faces).
    ny, nx = len(top) - 1, len(top[0]) - 1
    ring_top = [top[0][i] for i in range(nx)] + [top[j][nx] for j in range(ny)] + [top[ny][i] for i in range(nx, 0, -1)] + [top[j][0] for j in range(ny, 0, -1)]
    ring_bot = [bottom[0][i] for i in range(nx)] + [bottom[j][nx] for j in range(ny)] + [bottom[ny][i] for i in range(nx, 0, -1)] + [bottom[j][0] for j in range(ny, 0, -1)]
    for k in range(len(ring_top)):
        a, b = k, (k + 1) % len(ring_top)
        f = bm.faces.new((ring_bot[a], ring_bot[b], ring_top[b], ring_top[a]))
        f.material_index = 2
        for loop in f.loops:
            loop[uv].uv = (0.5, 0.5)
    for i, f in enumerate(bm.faces):
        if f.material_index != 2:
            f.material_index = 0 if i < n_top else 1
    obj = core.mesh_object('SM_Card', bm)
    for m in mats:
        obj.data.materials.append(m.m)
    return [obj]
