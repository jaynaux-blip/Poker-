"""ns_rooms: the rooms seen through Northside's kit windows (world realism spec 3.4 and 4.4, art-fronts).

Writes three textures to unreal/Art/Textures/NS (no meshes):

- T_NS_Rooms_Albedo.png (4096 x 4096, sRGB): 16 rooms in a 4 x 4 atlas, each the room by an overcast day (the light
  through its window and a little bounce, baked in; M_NS_WindowRoom scales it by MPC_NS.Daylight).
- T_NS_Rooms_Lamp.png (4096 x 4096, sRGB): the same 16 rooms lit by their own lamps alone (2700 K, Cycles), black
  where the lamps don't reach; the material adds it when the room is lit (Occupancy) and tints it by the hash.
- T_NS_RoomFront.png (4096 x 4096, sRGBA): 16 front layers hung in the window's own plane (curtains, blinds, plants,
  a taped sign, a cat): RGB their color by an overcast day, A their coverage (sheers about 0.45). Cell 15 is bare
  (alpha 0).

The cells. Cell i is column i % 4, row i // 4 counted from the TOP of the image; in UV (origin bottom-left, as the
material samples it) cell i covers u [col/4, (col+1)/4], v [1 - (row+1)/4, 1 - row/4]. Within a cell, (0, 0) is the
pane's bottom-left seen from outside, as the [window] slot's UV0 is.

  0-5   living rooms (warm beige, blue-grey, sage, exposed brick, mustard, white): sofas, a TV wall, shelving, art
  6-8   kitchens (white cabinets; green with a table; old oak with tile)
  9-11  bedrooms (a bed facing the window; a bed along the wall with a dresser; a kid's room with a desk)
  12    an office (desks, file cabinets, a drop ceiling with troffers)
  13    storage (boxes on steel shelving, a bare bulb)
  14    a stairwell landing (two-tone paint, a flight going up, a caged bare bulb)
  15    the library's reading room (shelves to the ceiling, a long oak table with green lamps); a shop's back room
        uses it at reduced scale

  RoomFront: 0 curtains drawn open (rust), 1 curtains drawn open (teal), 2 a sheer, 3 venetian blind half down,
  4 venetian blind down (slats open), 5 roller shade a third down, 6 a plant on the sill, 7 two plants and a jar,
  8 a cat on the sill (rare), 9 a taped FOR RENT sign, 10 cafe curtain on the lower half, 11 curtains and a plant,
  12 a blind fully down, slats shut, 13 a blanket tacked up as a curtain, 14 newspaper taped over (vacant), 15 bare.

How each room cell is made (spec 3.4): a one-point perspective render from a pinhole PIN = 1.0 window-width in front of
the opening, framed so the opening fills the cell exactly (the render's pixels are 1.5 times as tall as wide, so a
1.2 x 1.8 m opening fills the square cell). The room's back wall is at D = 3.2 m behind the opening, so M_NS_WindowRoom's
box (the pane's width and height, depth 3.2 m x a hash in 0.8..1.2) meets it exactly where it counts: u = 0.5 +
x * d / (d + z) with d = 1.0 window-width. The room itself is a real room, wider and taller than the opening (3.0-3.8 m
wide, its floor 0.85 m under the sill, its ceiling 0.3-0.5 m over the head), so its side walls, floor and ceiling show
round the opening the way they do through a real window.

NS_ROOMS_CELL=512 (environment) renders smaller cells for iterating; NS_ROOMS_ONLY=3,7 renders just those cells (the
others are kept from the last full atlas when it exists).
"""
import math
import os
import random

import bpy
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts, review
from artkit.sheet import Sheet

MESHES = []          # textures only (build.py exports nothing: see assemble)
NO_BAKE = True
TEX_DIR = os.path.join(core.ROOT, 'unreal', 'Art', 'Textures', 'NS')
CELL = int(os.environ.get('NS_ROOMS_CELL', 1024))
ONLY = [int(s) for s in os.environ.get('NS_ROOMS_ONLY', '').split(',') if s.strip()]
W, H = 1.2, 1.8      # the nominal opening (m)
D = 3.2              # the back wall's depth behind the opening
PIN = 1.0            # the pinhole's distance in front of the opening, in window widths
SILL = 0.85          # the room's floor under the opening's bottom
REVIEW_SCREEN_LIGHT = False
REVIEW_VIEWS = [('albedo', 0, 0, 0.75, (-6.0, 0.0, 0.0)), ('fronts', 0, 0, 0.75, (0.0, 0.0, 0.0)), ('lamp', 0, 0, 0.75, (6.0, 0.0, 0.0))]
_CARDS = []


def hx(h, a=1.0):
    return core.hex_linear(h, a)


def _tint(h, f):
    r, g, b = (h >> 16) & 255, (h >> 8) & 255, h & 255
    return (int(r + (255 - r) * f) << 16) | (int(g + (255 - g) * f) << 8) | int(b + (255 - b) * f)


def _shade(h, f):
    r, g, b = (h >> 16) & 255, (h >> 8) & 255, h & 255
    return (min(255, int(r * f)) << 16) | (min(255, int(g * f)) << 8) | min(255, int(b * f))


# ------------------------------------------------------------------ materials (rendered, never exported)

_MATS = {}
_LAMP_MATS = []      # shades and bulbs: emissive only in the lamp pass


def _mat(key, build):
    if key not in _MATS:
        _MATS[key] = build()
    return _MATS[key]


def paint(h, rough=0.85, mottle=0.06):
    """Flat wall paint, a little uneven (rollers, touch-ups, a hand's height of grubbiness)."""
    def b():
        m = core.Mat(f'paint_{h:06x}_{rough}')
        tc = m.node('ShaderNodeTexCoord')
        n = m.node('ShaderNodeTexNoise', Scale=1.6, Detail=3.0)
        m.link(tc.outputs['Object'], n.inputs['Vector'])
        m.set('Base Color', m.mix(m.math('MULTIPLY', n.outputs['Fac'], mottle * 4), hx(h), hx(_shade(h, 0.82))))
        m.set('Roughness', rough)
        return m
    return _mat(('paint', h, rough), b)


def matte(h, rough=0.6, metal=0.0):
    def b():
        m = core.Mat(f'matte_{h:06x}_{rough}_{metal}')
        m.set('Base Color', hx(h))
        m.set('Roughness', rough)
        m.set('Metallic', metal)
        return m
    return _mat(('matte', h, rough, metal), b)


def fabric(h, alt=None):
    def b():
        m = core.Mat(f'fabric_{h:06x}')
        tc = m.node('ShaderNodeTexCoord')
        n = m.node('ShaderNodeTexNoise', Scale=30.0, Detail=4.0)
        m.link(tc.outputs['Object'], n.inputs['Vector'])
        m.set('Base Color', m.mix(n.outputs['Fac'], hx(h), hx(alt if alt is not None else _shade(h, 0.75))))
        m.set('Roughness', 0.92)
        m.set('Sheen Weight', 0.6)
        return m
    return _mat(('fabric', h, alt), b)


def wood(h, scale=6.0):
    def b():
        m = core.Mat(f'wood_{h:06x}')
        tc = m.node('ShaderNodeTexCoord')
        wv = m.node('ShaderNodeTexWave', Scale=scale, Distortion=4.0, **{'Detail': 3.0})
        m.link(tc.outputs['Object'], wv.inputs['Vector'])
        m.set('Base Color', m.mix(wv.outputs['Fac'], hx(_shade(h, 0.7)), hx(h)))
        m.set('Roughness', 0.45)
        return m
    return _mat(('wood', h), b)


def boards(h, width=0.09):
    """Strip flooring: boards along the room's depth, each its own tone, the joints dark."""
    def b():
        m = core.Mat(f'boards_{h:06x}')
        tc = m.node('ShaderNodeTexCoord')
        sep = m.node('ShaderNodeSeparateXYZ')
        m.link(tc.outputs['Object'], sep.inputs['Vector'])
        br = m.node('ShaderNodeTexBrick', Scale=1.0, **{'Mortar Size': 0.003, 'Brick Width': 1.3, 'Row Height': width})
        br.offset = 0.37
        br.inputs['Color1'].default_value = hx(h)
        br.inputs['Color2'].default_value = hx(_shade(h, 0.72))
        br.inputs['Mortar'].default_value = hx(0x1a120c)
        cmb = m.node('ShaderNodeCombineXYZ')
        m.link(sep.outputs['Y'], cmb.inputs['X'])
        m.link(sep.outputs['X'], cmb.inputs['Y'])
        m.link(cmb.outputs['Vector'], br.inputs['Vector'])
        m.set('Base Color', br.outputs['Color'])
        m.set('Roughness', 0.35)
        return m
    return _mat(('boards', h, width), b)


def tiles(h, grout=0x8a867c, size=0.15, rough=0.2):
    def b():
        m = core.Mat(f'tiles_{h:06x}_{size}')
        tc = m.node('ShaderNodeTexCoord')
        sep = m.node('ShaderNodeSeparateXYZ')
        m.link(tc.outputs['Object'], sep.inputs['Vector'])
        cmb = m.node('ShaderNodeCombineXYZ')
        m.link(m.math('ADD', sep.outputs['X'], sep.outputs['Y']), cmb.inputs['X'])
        m.link(sep.outputs['Z'], cmb.inputs['Y'])
        br = m.node('ShaderNodeTexBrick', Scale=1.0, **{'Mortar Size': 0.004, 'Brick Width': size, 'Row Height': size * 0.5})
        br.inputs['Color1'].default_value = hx(h)
        br.inputs['Color2'].default_value = hx(_shade(h, 0.93))
        br.inputs['Mortar'].default_value = hx(grout)
        m.link(cmb.outputs['Vector'], br.inputs['Vector'])
        m.set('Base Color', br.outputs['Color'])
        m.set('Roughness', rough)
        return m
    return _mat(('tiles', h, size), b)


def checker(h0, h1, size=0.3):
    def b():
        m = core.Mat(f'checker_{h0:06x}')
        tc = m.node('ShaderNodeTexCoord')
        ck = m.node('ShaderNodeTexChecker', Scale=1.0 / size)
        m.link(tc.outputs['Object'], ck.inputs['Vector'])
        ck.inputs['Color1'].default_value = hx(h0)
        ck.inputs['Color2'].default_value = hx(h1)
        m.set('Base Color', ck.outputs['Color'])
        m.set('Roughness', 0.3)
        return m
    return _mat(('checker', h0, h1), b)


def brick_wall():
    def b():
        m = core.Mat('room_brick')
        tc = m.node('ShaderNodeTexCoord')
        sep = m.node('ShaderNodeSeparateXYZ')
        m.link(tc.outputs['Object'], sep.inputs['Vector'])
        cmb = m.node('ShaderNodeCombineXYZ')
        m.link(m.math('ADD', sep.outputs['X'], sep.outputs['Y']), cmb.inputs['X'])
        m.link(sep.outputs['Z'], cmb.inputs['Y'])
        br = m.node('ShaderNodeTexBrick', Scale=1.0, **{'Mortar Size': 0.008, 'Brick Width': 0.2, 'Row Height': 0.067})
        br.offset = 0.5
        br.inputs['Color1'].default_value = hx(0x7a3c2c)
        br.inputs['Color2'].default_value = hx(0x5c2c20)
        br.inputs['Mortar'].default_value = hx(0x9a9286)
        m.link(cmb.outputs['Vector'], br.inputs['Vector'])
        m.set('Base Color', br.outputs['Color'])
        m.set('Roughness', 0.85)
        return m
    return _mat(('brick',), b)


def rug(h0, h1, h2):
    """A rug: a border, a field and a lattice of medallions."""
    def b():
        m = core.Mat(f'rug_{h0:06x}')
        tc = m.node('ShaderNodeTexCoord')
        n = m.node('ShaderNodeTexWave', Scale=9.0, Distortion=2.0, **{'Detail': 2.0})
        n.wave_type = 'RINGS'
        m.link(tc.outputs['Object'], n.inputs['Vector'])
        ck = m.node('ShaderNodeTexChecker', Scale=6.0)
        m.link(tc.outputs['Object'], ck.inputs['Vector'])
        col = m.mix(ck.outputs['Fac'], hx(h0), hx(h1))
        col = m.mix(m.math('GREATER_THAN', n.outputs['Fac'], 0.7), col, hx(h2))
        m.set('Base Color', col)
        m.set('Roughness', 0.95)
        return m
    return _mat(('rug', h0, h1, h2), b)


def art(seed):
    """A painting or print in a frame: soft blocks of color."""
    def b():
        rnd = random.Random(seed)
        m = core.Mat(f'art_{seed}')
        tc = m.node('ShaderNodeTexCoord')
        n = m.node('ShaderNodeTexNoise', Scale=rnd.uniform(2.0, 6.0), Detail=2.0, Distortion=rnd.uniform(0.0, 2.0))
        m.link(tc.outputs['Object'], n.inputs['Vector'])
        pal = rnd.choice([(0x1f3b5a, 0xd9b26a, 0xc2462e), (0x2c4a2c, 0xe0d8c0, 0x7a2a2a), (0x111111, 0xe8e2d0, 0xd8a020),
                          (0x5a3a6a, 0xe8c0a0, 0x2a7a8a), (0x8a5a2a, 0xf0e8d8, 0x2a3a5a)])
        cr = m.node('ShaderNodeValToRGB')
        cr.color_ramp.elements[0].color = hx(pal[0])
        cr.color_ramp.elements[1].color = hx(pal[1])
        e = cr.color_ramp.elements.new(0.55)
        e.color = hx(pal[2])
        m.link(n.outputs['Fac'], cr.inputs['Fac'])
        m.set('Base Color', cr.outputs['Color'])
        m.set('Roughness', 0.5)
        return m
    return _mat(('art', seed), b)


def books_mat(seed):
    """Book spines: thin vertical bands of color along a shelf (x), varying in height a little."""
    def b():
        m = core.Mat(f'books_{seed}')
        tc = m.node('ShaderNodeTexCoord')
        sep = m.node('ShaderNodeSeparateXYZ')
        m.link(tc.outputs['Object'], sep.inputs['Vector'])
        along = m.math('ADD', sep.outputs['X'], sep.outputs['Y'])
        wn = m.node('ShaderNodeTexWhiteNoise')
        wn.noise_dimensions = '1D'
        m.link(m.math('FLOOR', m.math('MULTIPLY', along, 33.0 + seed)), wn.inputs['W'])
        cr = m.node('ShaderNodeValToRGB')
        cols = [0x7a1f1a, 0x1f3a5a, 0xd8cdb0, 0x2a4a2a, 0x1a1a1a, 0xb08a3a, 0x6a2a4a, 0xe0dccc, 0x3a5a7a]
        cr.color_ramp.interpolation = 'CONSTANT'
        cr.color_ramp.elements[0].color = hx(cols[0])
        cr.color_ramp.elements[1].position = 0.95
        cr.color_ramp.elements[1].color = hx(cols[-1])
        for i, c in enumerate(cols[1:-1]):
            e = cr.color_ramp.elements.new((i + 1) / len(cols))
            e.color = hx(c)
        m.link(wn.outputs['Value'], cr.inputs['Fac'])
        gap = m.math('GREATER_THAN', m.math('FRACT', m.math('MULTIPLY', along, 33.0 + seed)), 0.93)
        m.set('Base Color', m.mix(gap, cr.outputs['Color'], hx(0x0c0a08)))
        m.set('Roughness', 0.6)
        return m
    return _mat(('books', seed), b)


def screen():
    def b():
        m = core.Mat('room_screen')
        m.set('Base Color', hx(0x050608))
        m.set('Roughness', 0.12)
        return m
    return _mat(('screen',), b)


def lampshade(h, glow=6.0):
    """A shade or a bulb: diffuse by day, glowing in the lamp pass (2700 K)."""
    m = core.Mat(f'shade_{h:06x}_{len(_LAMP_MATS)}')
    m.set('Base Color', hx(h))
    m.set('Roughness', 0.7)
    m.set('Emission Color', hx(0xffb46b))
    m.set('Emission Strength', 0.0)
    _LAMP_MATS.append((m, glow))
    return m


def sheer(h, alpha):
    """A see-through fabric (front layers): transparent in proportion."""
    m = core.Mat(f'sheer_{h:06x}_{alpha}')
    m.set('Base Color', hx(h))
    m.set('Roughness', 0.9)
    m.set('Alpha', alpha)
    return m


# ------------------------------------------------------------------ geometry helpers (room frame: x across the window,
# left to right seen from outside; y into the room, 0 at the opening; z up, 0 at the opening's bottom)

class Room:
    def __init__(self, cell, width=3.4, x_off=0.0, height=3.0, depth=D):
        self.cell = cell
        self.rnd = random.Random(4100 + cell)
        self.x0, self.x1 = -width / 2 + x_off, width / 2 + x_off
        self.fz, self.cz = -SILL, -SILL + height
        self.d = depth
        self.objs = []
        self.lights = []

    def box(self, x0, x1, y0, y1, z0, z1, mat, r=0.0, seg=2):
        o = parts.rbox('b', ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), r, seg)
        core.assign(o, mat)
        self.objs.append(o)
        return o

    def rod(self, a, b, r, mat, n=12):
        o = parts.rod('r', a, b, r, n)
        core.assign(o, mat)
        self.objs.append(o)
        return o

    def lathe(self, prof, at, mat, n=32):
        o = parts.lathe_at('l', prof, Matrix.Translation(at), n)
        core.assign(o, mat)
        self.objs.append(o)
        return o

    def shell(self, wall, floor, ceil, back=None, left=None, right=None, base=0xe8e2d4, crown=True):
        """Floor, ceiling, the three walls, the window wall (its opening exactly the frame), baseboards and crown."""
        x0, x1, fz, cz, d = self.x0, self.x1, self.fz, self.cz, self.d
        t = 0.1
        self.box(x0 - t, x1 + t, -0.05, d + t, fz - t, fz, floor)
        self.box(x0 - t, x1 + t, -0.05, d + t, cz, cz + t, ceil)
        self.box(x0 - t, x1 + t, d, d + t, fz, cz, back or wall)
        self.box(x0 - t, x0, -0.05, d, fz, cz, left or wall)
        self.box(x1, x1 + t, -0.05, d, fz, cz, right or wall)
        # The window wall, outside the frame: a 0.3 m wall with the opening through it.
        for a, b, c, e in ((x0 - t, -W / 2, fz, cz), (W / 2, x1 + t, fz, cz), (-W / 2, W / 2, fz, 0.0), (-W / 2, W / 2, H, cz)):
            self.box(a, b, -0.3, 0.0, c, e, wall)
        bm = matte(base, 0.4)
        self.box(x0, x1, d - 0.015, d, fz, fz + 0.12, bm)
        self.box(x0, x0 + 0.015, 0.0, d, fz, fz + 0.12, bm)
        self.box(x1 - 0.015, x1, 0.0, d, fz, fz + 0.12, bm)
        if crown:
            self.box(x0, x1, d - 0.04, d, cz - 0.08, cz, bm)
            self.box(x0, x0 + 0.04, 0.0, d, cz - 0.08, cz, bm)
            self.box(x1 - 0.04, x1, 0.0, d, cz - 0.08, cz, bm)

    def door(self, x, wall='left', y=1.9, colour=0xd8d2c4, open_=False):
        """A panelled door on a side wall (its casing), or open on a dark hall."""
        fz = self.fz
        s = 1 if wall == 'left' else -1
        wx = self.x0 if wall == 'left' else self.x1
        cm = matte(0xece6d8, 0.4)
        self.box(wx, wx + s * 0.02, y - 0.5, y - 0.42, fz, fz + 2.15, cm)
        self.box(wx, wx + s * 0.02, y + 0.42, y + 0.5, fz, fz + 2.15, cm)
        self.box(wx, wx + s * 0.02, y - 0.5, y + 0.5, fz + 2.1, fz + 2.2, cm)
        if open_:
            self.box(wx - s * 0.001, wx + s * 0.005, y - 0.42, y + 0.42, fz, fz + 2.1, matte(0x15120f, 0.9))
        else:
            dm = matte(colour, 0.45)
            self.box(wx, wx + s * 0.035, y - 0.42, y + 0.42, fz, fz + 2.1, dm)
            for zz in (0.25, 1.15):
                self.box(wx + s * 0.035, wx + s * 0.045, y - 0.3, y + 0.3, fz + zz, fz + zz + 0.75, dm, 0.01)
            self.lathe([(0.0, 0.0), (0.03, 0.0), (0.03, 0.02), (0.0, 0.03)], (wx + s * 0.06, y + 0.32, fz + 1.0), matte(0xb8913a, 0.3, 1.0), 12)

    def picture(self, x, y, z, w, h, wall='back', seed=1, frame=0x2a1c12):
        fm = matte(frame, 0.4)
        if wall == 'back':
            d = self.d
            self.box(x - w / 2, x + w / 2, d - 0.03, d, z - h / 2, z + h / 2, fm)
            self.box(x - w / 2 + 0.04, x + w / 2 - 0.04, d - 0.035, d - 0.03, z - h / 2 + 0.04, z + h / 2 - 0.04, art(seed))
        else:
            s = 1 if wall == 'left' else -1
            wx = self.x0 if wall == 'left' else self.x1
            self.box(wx, wx + s * 0.03, y - w / 2, y + w / 2, z - h / 2, z + h / 2, fm)
            self.box(wx + s * 0.03, wx + s * 0.035, y - w / 2 + 0.04, y + w / 2 - 0.04, z - h / 2 + 0.04, z + h / 2 - 0.04, art(seed))

    def light(self, at, power=60.0, radius=0.06, temp=2700, kind='POINT', size=0.3, rot=None):
        data = bpy.data.lights.new('lamp', kind)
        data.energy = power
        data.color = blackbody(temp)
        if kind == 'POINT':
            data.shadow_soft_size = radius
        elif kind == 'AREA':
            data.size = size
        o = core.link(bpy.data.objects.new('lamp', data))
        o.visible_camera = False
        o.location = at
        if rot is not None:
            o.rotation_euler = rot
        self.lights.append(o)
        return o

    # Furniture.
    def sofa(self, x, y, facing, h=0x6a4a3a, w=1.9):
        """A sofa centered at (x, y) facing -y (toward the window) or +x / -x."""
        f = fabric(h)
        fz = self.fz
        if facing == '-y':
            self.box(x - w / 2, x + w / 2, y - 0.45, y + 0.45, fz + 0.1, fz + 0.45, f, 0.04)
            self.box(x - w / 2, x + w / 2, y + 0.22, y + 0.45, fz + 0.45, fz + 0.85, f, 0.06)
            for s in (-1, 1):
                self.box(x + s * (w / 2 - 0.1), x + s * (w / 2 + 0.06), y - 0.45, y + 0.45, fz + 0.1, fz + 0.66, f, 0.05)
            for i in range(3):
                cx = x - w / 2 + 0.16 + (w - 0.32) * (i + 0.5) / 3
                self.box(cx - (w - 0.36) / 6, cx + (w - 0.36) / 6, y - 0.42, y + 0.2, fz + 0.45, fz + 0.58, f, 0.05)
            self.box(x - 0.55, x - 0.15, y + 0.1, y + 0.22, fz + 0.55, fz + 0.88, fabric(0xc8a24a), 0.05)
        else:
            s = 1 if facing == '+x' else -1
            self.box(x - 0.45, x + 0.45, y - w / 2, y + w / 2, fz + 0.1, fz + 0.45, f, 0.04)
            self.box(x - s * 0.45, x - s * 0.22, y - w / 2, y + w / 2, fz + 0.45, fz + 0.85, f, 0.06)
            for t in (-1, 1):
                self.box(x - 0.45, x + 0.45, y + t * (w / 2 - 0.1), y + t * (w / 2 + 0.06), fz + 0.1, fz + 0.66, f, 0.05)
            self.box(x - s * 0.2, x + s * 0.42, y - w / 2 + 0.15, y + w / 2 - 0.15, fz + 0.45, fz + 0.58, f, 0.05)
        for dx in (-1, 1):
            for dy in (-1, 1):
                self.box(x + dx * (w / 2 - 0.05 if facing == '-y' else 0.4) - 0.02, x + dx * (w / 2 - 0.05 if facing == '-y' else 0.4) + 0.02,
                         y + dy * (0.4 if facing == '-y' else w / 2 - 0.05) - 0.02, y + dy * (0.4 if facing == '-y' else w / 2 - 0.05) + 0.02,
                         fz, fz + 0.1, matte(0x1a120c, 0.5))

    def armchair(self, x, y, h=0x8a3a2a):
        f = fabric(h)
        fz = self.fz
        self.box(x - 0.42, x + 0.42, y - 0.4, y + 0.4, fz + 0.12, fz + 0.45, f, 0.04)
        self.box(x - 0.42, x + 0.42, y + 0.2, y + 0.42, fz + 0.45, fz + 0.95, f, 0.06)
        for s in (-1, 1):
            self.box(x + s * 0.32, x + s * 0.44, y - 0.4, y + 0.42, fz + 0.12, fz + 0.66, f, 0.04)

    def table(self, x0, x1, y0, y1, top_z, mat, legs=0.04):
        fz = self.fz
        self.box(x0, x1, y0, y1, top_z - 0.04, top_z, mat, 0.006)
        for x in (x0 + 0.05, x1 - 0.05):
            for y in (y0 + 0.05, y1 - 0.05):
                self.box(x - legs / 2, x + legs / 2, y - legs / 2, y + legs / 2, fz, top_z - 0.04, mat)

    def shelf(self, x0, x1, y0, y1, z0, z1, rows=5, mat=None, seed=1, books=0.8):
        """A bookcase against a wall: carcass, shelves, books on most."""
        mat = mat or wood(0x5a3a22)
        self.box(x0, x1, y0, y1, z0, z0 + 0.02, mat)
        self.box(x0, x1, y0, y1, z1 - 0.02, z1, mat)
        across_x = (x1 - x0) > (y1 - y0)
        if across_x:
            self.box(x0, x0 + 0.02, y0, y1, z0, z1, mat)
            self.box(x1 - 0.02, x1, y0, y1, z0, z1, mat)
            self.box(x0, x1, y1 - 0.01, y1, z0, z1, mat)
        else:
            self.box(x0, x1, y0, y0 + 0.02, z0, z1, mat)
            self.box(x0, x1, y1 - 0.02, y1, z0, z1, mat)
            back = x1 - 0.01 if x1 > 0 else x0
            self.box(back, back + 0.01, y0, y1, z0, z1, mat)
        rnd = random.Random(seed)
        for i in range(rows):
            z = z0 + 0.04 + (z1 - z0 - 0.06) * i / rows
            zt = z0 + 0.04 + (z1 - z0 - 0.06) * (i + 1) / rows
            self.box(x0, x1, y0, y1, z - 0.02, z, mat)
            if rnd.random() < books:
                bh = min(zt - z - 0.04, rnd.uniform(0.18, 0.26))
                fill = rnd.uniform(0.55, 0.95)
                if across_x:
                    self.box(x0 + 0.03, x0 + 0.03 + (x1 - x0 - 0.06) * fill, y0 + 0.04, y1 - 0.03, z, z + bh, books_mat(rnd.randint(0, 9)))
                else:
                    self.box(x0 + 0.04, x1 - 0.03, y0 + 0.03, y0 + 0.03 + (y1 - y0 - 0.06) * fill, z, z + bh, books_mat(rnd.randint(0, 9)))

    def floor_lamp(self, x, y, shade_hex=0xe8d8b0, power=40.0):
        fz = self.fz
        mt = matte(0x2a2620, 0.4, 0.8)
        self.lathe([(0.0, 0.0), (0.15, 0.0), (0.14, 0.03), (0.0, 0.04)], (x, y, fz), mt, 24)
        self.rod((x, y, fz), (x, y, fz + 1.45), 0.012, mt)
        self.lathe([(0.0, 0.0), (0.21, 0.0), (0.13, 0.32), (0.0, 0.32)], (x, y, fz + 1.35), lampshade(shade_hex), 32)
        self.light((x, y, fz + 1.5), power, 0.08)

    def table_lamp(self, x, y, z, shade_hex=0xe8d8b0, power=25.0):
        self.lathe([(0.0, 0.0), (0.08, 0.0), (0.09, 0.12), (0.03, 0.3), (0.0, 0.3)], (x, y, z), matte(0x6a5a40, 0.3), 24)
        self.lathe([(0.0, 0.0), (0.16, 0.0), (0.11, 0.22), (0.0, 0.22)], (x, y, z + 0.28), lampshade(shade_hex), 32)
        self.light((x, y, z + 0.4), power, 0.06)

    def pendant(self, x, y, drop=0.55, shade_hex=0xf0e6d0, power=50.0, kind='dome'):
        cz = self.cz
        self.rod((x, y, cz), (x, y, cz - drop), 0.005, matte(0x111111))
        if kind == 'dome':
            self.lathe([(0.0, 0.0), (0.25, 0.0), (0.22, 0.12), (0.12, 0.2), (0.0, 0.21)], (x, y, cz - drop - 0.18), lampshade(shade_hex), 32)
        elif kind == 'globe':
            o = parts.rbox('g', (x, y, cz - drop - 0.12), (0.26, 0.26, 0.26), 0.13, 6)
            core.assign(o, lampshade(shade_hex))
            self.objs.append(o)
        else:
            self.lathe([(0.0, 0.0), (0.03, 0.0), (0.035, 0.06), (0.0, 0.09)], (x, y, cz - drop - 0.06), lampshade(0xfff4d8, 30.0), 16)
        self.light((x, y, cz - drop - 0.2), power, 0.05)

    def ceiling_fixture(self, x, y, power=70.0):
        cz = self.cz
        self.lathe([(0.0, -0.08), (0.18, -0.06), (0.2, 0.0), (0.0, 0.0)], (x, y, cz), lampshade(0xf4efe4), 32)
        self.light((x, y, cz - 0.15), power, 0.12)

    def tv(self, x, y, wall='left', w=1.1):
        """A flat screen on a low stand against a side wall (switched off by day: the material does the flicker)."""
        fz = self.fz
        s = 1 if wall == 'left' else -1
        wx = self.x0 if wall == 'left' else self.x1
        self.box(wx, wx + s * 0.45, y - 0.8, y + 0.8, fz, fz + 0.48, wood(0x3a2a1c))
        self.box(wx + s * 0.08, wx + s * 0.12, y - w / 2, y + w / 2, fz + 0.62, fz + 0.62 + w * 0.58, matte(0x0d0d0e, 0.3))
        self.box(wx + s * 0.12, wx + s * 0.125, y - w / 2 + 0.02, y + w / 2 - 0.02, fz + 0.64, fz + 0.6 + w * 0.58, screen())
        self.box(wx + s * 0.08, wx + s * 0.2, y - 0.1, y + 0.1, fz + 0.48, fz + 0.62, matte(0x111111))

    def plant(self, x, y, z, s=1.0):
        pot = matte(0x9a5234, 0.7)
        self.lathe([(0.0, 0.0), (0.1 * s, 0.0), (0.13 * s, 0.22 * s), (0.0, 0.22 * s)], (x, y, z), pot, 20)
        leaf = matte(0x2c5a26, 0.6)
        rnd = random.Random(int(x * 100 + y * 10))
        for i in range(9):
            a = i * 2.4
            r = rnd.uniform(0.12, 0.28) * s
            tip = (x + math.cos(a) * r, y + math.sin(a) * r, z + rnd.uniform(0.35, 0.75) * s)
            o = parts.rbox('leaf', tip, (0.16 * s, 0.05 * s, 0.1 * s), 0.02, 2)
            o.data.transform(Matrix.Translation(tip) @ Matrix.Rotation(a, 4, 'Z') @ Matrix.Translation(Vector(tip) * -1))
            core.assign(o, leaf)
            self.objs.append(o)
            self.rod((x, y, z + 0.2 * s), tip, 0.006, leaf, 5)

    def window_side(self, wall, y, z0=0.0, z1=H):
        """A window in a side wall (night blue beyond), its casing."""
        s = 1 if wall == 'left' else -1
        wx = self.x0 if wall == 'left' else self.x1
        self.box(wx - s * 0.002, wx + s * 0.002, y - 0.5, y + 0.5, z0, z1, matte(0x4a5260, 0.1))
        cm = matte(0xece6d8, 0.4)
        for a, b, c, e in ((y - 0.56, y - 0.5, z0, z1), (y + 0.5, y + 0.56, z0, z1), (y - 0.56, y + 0.56, z1, z1 + 0.08), (y - 0.56, y + 0.56, z0 - 0.06, z0)):
            self.box(wx, wx + s * 0.03, a, b, c, e, cm)


def blackbody(k):
    """Approximate sRGB-linear color of a blackbody at k kelvin (normalized)."""
    t = k / 100.0
    r = 255.0 if t <= 66 else 329.698727446 * ((t - 60) ** -0.1332047592)
    g = 99.4708025861 * math.log(t) - 161.1195681661 if t <= 66 else 288.1221695283 * ((t - 60) ** -0.0755148492)
    b = 255.0 if t >= 66 else (0.0 if t <= 19 else 138.5177312231 * math.log(t - 10) - 305.0447927307)
    c = [max(0.0, min(255.0, v)) / 255.0 for v in (r, g, b)]
    return tuple(core.srgb_to_linear(v) for v in c)


# ------------------------------------------------------------------ the sixteen rooms

def living(cell):
    """Cells 0-5: living rooms, each its own palette and arrangement."""
    looks = [
        dict(wall=0xd8c8a8, floor=0x6a4228, sofa=0x5a3a2c, rug=(0x7a2a22, 0xb08a5a, 0x1f2a3a), x_off=0.4, tv=None, shelf='left', lamp='floor'),
        dict(wall=0x9aa6b0, floor=0x5a3a24, sofa=0x2e3a4a, rug=(0x3a4a5a, 0xd0c8b0, 0x7a2a2a), x_off=-0.3, tv='left', shelf=None, lamp='table'),
        dict(wall=0xa8b496, floor=0x7a5232, sofa=0x8a7a5a, rug=(0x4a3a2a, 0xc8b88a, 0x2a4a3a), x_off=0.0, tv=None, shelf='right', lamp='pendant'),
        dict(wall='brick', floor=0x4a2e1c, sofa=0x2a2a2a, rug=(0x2a2a2a, 0x8a8a7a, 0xb03a2a), x_off=0.6, tv='right', shelf='back', lamp='floor'),
        dict(wall=0xd6b45a, floor=0x6a4630, sofa=0x3a5a4a, rug=(0x8a3a2a, 0xe0c890, 0x2a3a2a), x_off=-0.5, tv=None, shelf='back', lamp='table'),
        dict(wall=0xe4e0d8, floor=0x8a6a48, sofa=0x6a6a6a, rug=(0x1a1a1a, 0xe8e4dc, 0x3a6a8a), x_off=0.2, tv='left', shelf=None, lamp='pendant'),
    ]
    L = looks[cell]
    R = Room(cell, width=3.6, x_off=L['x_off'], height=3.0)
    wall = brick_wall() if L['wall'] == 'brick' else paint(L['wall'])
    R.shell(wall, boards(L['floor']), paint(0xece8e0, 0.9), back=wall)
    d = R.d
    fz = R.fz
    # Rug, sofa against the back wall facing the window, a coffee table.
    rugm = rug(*L['rug'])
    R.box(-1.1 + L['x_off'] * 0.3, 1.1 + L['x_off'] * 0.3, 0.9, 2.7, fz, fz + 0.008, rugm)
    sx = R.rnd.uniform(-0.35, 0.35) + L['x_off'] * 0.3
    R.sofa(sx, d - 0.5, '-y', L['sofa'], 2.0)
    R.table(sx - 0.55, sx + 0.55, 1.7, 2.2, fz + 0.42, wood(0x4a2e1a))
    R.box(sx - 0.2, sx + 0.1, 1.85, 2.05, fz + 0.42, fz + 0.45, matte(0xc8bfa8, 0.6))
    R.picture(sx, None, fz + 1.65, 1.0, 0.7, 'back', seed=cell * 7 + 1)
    if cell % 2 == 0:
        R.picture(sx + 1.0, None, fz + 1.55, 0.4, 0.5, 'back', seed=cell * 7 + 2, frame=0xd8d0c0)
    if L['tv']:
        R.tv(None, 1.7, L['tv'])
        R.armchair(R.x1 - 0.6 if L['tv'] == 'left' else R.x0 + 0.6, 1.0, 0x7a2a1e if cell != 5 else 0x3a3a3a)
    else:
        R.armchair(R.x0 + 0.55, 1.4, [0x7a2a1e, 0x2a4a5a, 0x8a6a2a, 0x3a3a3a, 0x6a2a3a, 0x4a4a4a][cell])
    if L['shelf'] == 'left':
        R.shelf(R.x0, R.x0 + 0.35, 1.5, 2.6, fz, fz + 2.1, 6, seed=cell)
    elif L['shelf'] == 'right':
        R.shelf(R.x1 - 0.35, R.x1, 1.2, 2.3, fz, fz + 2.1, 6, seed=cell)
    elif L['shelf'] == 'back':
        R.shelf(R.x1 - 1.1, R.x1 - 0.1, d - 0.35, d, fz, fz + 1.9, 5, seed=cell, mat=matte(0xe8e4dc, 0.5) if cell == 3 else None)
    # A door to the hall on one side wall, a picture on the other.
    R.door(None, 'right' if L['shelf'] != 'right' and L['tv'] != 'right' else 'left', y=0.8, open_=cell % 3 == 1)
    R.picture(None, 2.2, fz + 1.6, 0.6, 0.45, 'left' if L['tv'] != 'left' and L['shelf'] != 'left' else 'right', seed=cell * 7 + 3)
    R.plant(R.x0 + 0.35, d - 0.35, fz, 1.6)
    if L['lamp'] == 'floor':
        R.floor_lamp(sx + 1.25, d - 0.4, 0xe8d0a0, 45.0)
    elif L['lamp'] == 'table':
        R.box(sx - 1.4, sx - 1.0, d - 0.45, d - 0.05, fz, fz + 0.55, wood(0x3a2614))
        R.table_lamp(sx - 1.2, d - 0.25, fz + 0.55, 0xf0e0c0, 30.0)
    else:
        R.pendant(sx, 2.0, 0.6, 0xe8dcc0, 70.0)
    R.ceiling_fixture(L['x_off'] * 0.3, 1.2, 15.0)
    return R


def kitchen(cell):
    """Cells 6-8: kitchens."""
    k = cell - 6
    looks = [dict(wall=0xe8e4d8, cab=0xf0ece4, top=0x2a2a2a, floor=('checker', 0xe8e4da, 0x1a1a1a), splash=0xe8e8e8),
             dict(wall=0xe8dcc0, cab=0x4a6a4a, top=0xd8d0c0, floor=('boards', 0x8a6a48), splash=0xd8e0d8),
             dict(wall=0xd8c8a0, cab=0x7a5232, top=0xc8c0b0, floor=('tiles', 0xb0583a), splash=0xf0ead8)]
    L = looks[k]
    R = Room(cell, width=3.0, x_off=(-0.3, 0.4, 0.1)[k], height=2.9)
    fl = L['floor']
    floor = checker(fl[1], fl[2]) if fl[0] == 'checker' else (boards(fl[1]) if fl[0] == 'boards' else tiles(fl[1], 0x6a5a4a, 0.3, 0.4))
    R.shell(paint(L['wall']), floor, paint(0xf4f0e8, 0.9))
    d, fz = R.d, R.fz
    cab, top = matte(L['cab'], 0.4), matte(L['top'], 0.3)
    # Base cabinets and counter along the back wall, uppers over, the backsplash between.
    x0, x1 = R.x0 + 0.05, R.x1 - 0.75
    R.box(x0, x1, d - 0.6, d, fz + 0.1, fz + 0.88, cab, 0.005)
    R.box(x0, x1, d - 0.62, d - 0.6, fz, fz + 0.1, matte(0x1a1a1a))
    R.box(x0 - 0.02, x1 + 0.02, d - 0.63, d, fz + 0.88, fz + 0.92, top, 0.004)
    n = max(2, int((x1 - x0) / 0.5))
    for i in range(n):
        a = x0 + (x1 - x0) * i / n
        b = x0 + (x1 - x0) * (i + 1) / n
        R.box(a + 0.01, b - 0.01, d - 0.605, d - 0.6, fz + 0.15, fz + 0.86, cab, 0.004)
        R.box((a + b) / 2 - 0.05, (a + b) / 2 + 0.05, d - 0.62, d - 0.605, fz + 0.78, fz + 0.8, matte(0xb0b0b0, 0.3, 1.0))
        R.box(a + 0.01, b - 0.01, d - 0.35, d, fz + 1.45, fz + 2.15, cab, 0.004)
    R.box(x0, x1, d - 0.012, d, fz + 0.92, fz + 1.45, tiles(L['splash'], 0x9a968e, 0.15, 0.15))
    # Sink, kettle, a bowl of fruit.
    sxm = (x0 + x1) / 2
    R.box(sxm - 0.3, sxm + 0.3, d - 0.55, d - 0.12, fz + 0.9, fz + 0.925, matte(0xc0c4c8, 0.25, 1.0))
    R.rod((sxm, d - 0.08, fz + 0.92), (sxm, d - 0.08, fz + 1.15), 0.012, matte(0xc0c4c8, 0.2, 1.0))
    R.rod((sxm, d - 0.08, fz + 1.15), (sxm, d - 0.25, fz + 1.12), 0.01, matte(0xc0c4c8, 0.2, 1.0))
    R.lathe([(0.0, 0.0), (0.09, 0.0), (0.1, 0.15), (0.05, 0.2), (0.0, 0.21)], (x0 + 0.3, d - 0.3, fz + 0.92), matte(0xb03a2a, 0.3), 20)
    # The fridge in the corner, the stove beside it.
    R.box(R.x1 - 0.72, R.x1 - 0.02, d - 0.72, d, fz, fz + 1.75, matte(0xe8e8e4 if k != 2 else 0xd8d0b0, 0.3), 0.02)
    R.box(R.x1 - 0.7, R.x1 - 0.66, d - 0.74, d - 0.72, fz + 1.0, fz + 1.5, matte(0x9a9a9a, 0.3, 1.0))
    if k != 1:
        R.box(x1 - 0.65, x1 - 0.05, d - 0.61, d - 0.005, fz + 0.88, fz + 0.94, matte(0x1a1a1a, 0.3))
        for dx in (-0.15, 0.15):
            for dy in (-0.15, 0.15):
                R.lathe([(0.0, 0.0), (0.08, 0.0), (0.0, 0.01)], (x1 - 0.35 + dx, d - 0.3 + dy, fz + 0.94), matte(0x333333, 0.4), 16)
    # A table and chairs, or a stool.
    if k == 1:
        R.table(-0.5, 0.4, 1.1, 1.9, fz + 0.75, wood(0xb08a5a))
        for x, y in ((-0.05, 0.85), (-0.05, 2.15)):
            R.box(x - 0.2, x + 0.2, y - 0.2, y + 0.2, fz + 0.45, fz + 0.49, wood(0x7a5232))
            for dx in (-0.17, 0.17):
                for dy in (-0.17, 0.17):
                    R.box(x + dx - 0.015, x + dx + 0.015, y + dy - 0.015, y + dy + 0.015, fz, fz + 0.45, wood(0x7a5232))
        R.pendant(-0.05, 1.5, 0.75, 0xd8e8d0, 65.0)
    else:
        R.ceiling_fixture((x0 + x1) / 2, 1.8, 80.0)
    R.picture(None, 1.0, fz + 1.55, 0.35, 0.45, 'left', seed=cell * 5)
    R.box(R.x0, R.x0 + 0.02, 1.6, 1.62, fz + 1.2, fz + 1.28, matte(0xe8e4dc, 0.4))
    # Under-cabinet light over the counter.
    R.light(((x0 + x1) / 2, d - 0.3, fz + 1.4), 18.0, 0.2)
    return R


def bedroom(cell):
    """Cells 9-11: bedrooms."""
    k = cell - 9
    looks = [dict(wall=0xb8c4cc, floor=0x6a4a30, spread=0xd8d4c8, alt=0x2a3a5a),
             dict(wall=0xd8b8a8, floor=0x7a5a3a, spread=0x7a2a2a, alt=0xd8c8a8),
             dict(wall=0xa8c8b8, floor=0x8a6a4a, spread=0x3a6a9a, alt=0xe8d040)]
    L = looks[k]
    R = Room(cell, width=3.4, x_off=(0.2, -0.4, 0.3)[k], height=2.9)
    R.shell(paint(L['wall']), boards(L['floor']), paint(0xf0ece4, 0.9))
    d, fz = R.d, R.fz
    spread, wood_ = fabric(L['spread'], _shade(L['spread'], 0.8)), wood(0x5a3a22)
    if k == 1:
        # Along the right wall, its head at the back.
        bx0, bx1 = R.x1 - 1.45, R.x1 - 0.05
        R.box(bx0, bx1, d - 2.05, d - 0.06, fz + 0.15, fz + 0.5, matte(0xe8e4dc, 0.8), 0.06)
        R.box(bx0 - 0.02, bx1 + 0.02, d - 2.0, d - 0.5, fz + 0.45, fz + 0.58, spread, 0.06)
        R.box(bx0, bx1, d - 0.08, d, fz, fz + 1.1, wood_, 0.02)
        for x in (bx0 + 0.35, bx1 - 0.35):
            R.box(x - 0.3, x + 0.3, d - 0.5, d - 0.1, fz + 0.5, fz + 0.66, fabric(0xf0ece4), 0.07)
        R.box(R.x0, R.x0 + 0.5, 1.2, 2.2, fz, fz + 1.1, wood(0x6a4a2a), 0.01)
        for i in range(4):
            R.box(R.x0 + 0.5, R.x0 + 0.505, 1.25, 2.15, fz + 0.1 + i * 0.25, fz + 0.32 + i * 0.25, wood(0x7a5a3a))
        R.box(R.x0, R.x0 + 0.03, 1.3, 2.1, fz + 1.3, fz + 2.1, matte(0x8a9aa8, 0.05, 1.0))
        R.table_lamp(R.x0 + 0.25, 2.0, fz + 1.1, 0xe8d8c0, 25.0)
    else:
        bx = R.rnd.uniform(-0.3, 0.3)
        w = 1.5 if k == 0 else 1.0
        R.box(bx - w / 2, bx + w / 2, d - 2.05, d - 0.06, fz + 0.15, fz + 0.5, matte(0xe8e4dc, 0.8), 0.06)
        R.box(bx - w / 2 - 0.02, bx + w / 2 + 0.02, d - 2.0, d - 0.55, fz + 0.45, fz + 0.6, spread, 0.06)
        R.box(bx - w / 2 - 0.05, bx + w / 2 + 0.05, d - 0.08, d, fz, fz + 1.25, wood_ if k == 0 else matte(0xe8e4dc, 0.4), 0.03)
        for x in ((bx - 0.38, bx + 0.38) if k == 0 else (bx,)):
            R.box(x - 0.3, x + 0.3, d - 0.5, d - 0.1, fz + 0.5, fz + 0.66, fabric(0xf4f0e8), 0.07)
        for s in (-1, 1):
            nx = bx + s * (w / 2 + 0.3)
            R.box(nx - 0.22, nx + 0.22, d - 0.45, d - 0.02, fz, fz + 0.55, wood_, 0.01)
        R.table_lamp(bx - (w / 2 + 0.3), d - 0.25, fz + 0.55, 0xf0e0c8, 22.0)
        R.picture(bx, None, fz + 1.65, 0.9 if k == 0 else 0.5, 0.55, 'back', seed=cell * 3)
        if k == 2:
            R.table(R.x0 + 0.05, R.x0 + 0.65, 0.9, 2.0, fz + 0.74, matte(0xe8e4dc, 0.4))
            R.box(R.x0 + 0.15, R.x0 + 0.5, 1.3, 1.6, fz + 0.74, fz + 0.76, matte(0x2a2a2a, 0.3))
            for i, h in enumerate((0xd83a3a, 0x3a7ad8, 0xe8c83a)):
                R.box(R.x0, R.x0 + 0.005, 0.7 + i * 0.55, 1.15 + i * 0.55, fz + 1.4, fz + 2.0, matte(h, 0.6))
    R.door(None, 'left' if k != 1 else 'right', y=0.7, open_=k == 0)
    R.ceiling_fixture(0.0, 1.5, 12.0)
    return R


def office(cell):
    R = Room(cell, width=4.2, x_off=0.3, height=2.8)
    R.shell(paint(0xd8d4c8), tiles(0x6a6a64, 0x4a4a46, 0.45, 0.6), matte(0xe8e6e0, 0.9), crown=False)
    d, fz, cz = R.d, R.fz, R.cz
    # Drop ceiling grid and two troffers.
    for i in range(-4, 5):
        R.box(i * 0.6 - 0.01, i * 0.6 + 0.01, 0.0, d, cz - 0.02, cz, matte(0xd0d0cc, 0.4, 0.5))
    for j in range(6):
        R.box(R.x0, R.x1, j * 0.6 - 0.01, j * 0.6 + 0.01, cz - 0.02, cz, matte(0xd0d0cc, 0.4, 0.5))
    for x, y in ((-0.3, 1.2), (0.9, 2.4)):
        R.box(x - 0.3, x + 0.3, y - 0.6, y + 0.6, cz - 0.03, cz - 0.02, lampshade(0xf4f6f8, 4.0))
        R.light((x, y, cz - 0.1), 60.0, kind='AREA', size=0.6)
    # Desks with monitors, chairs, file cabinets along the back.
    for x, y in ((-0.6, 1.4), (0.8, 2.0)):
        R.table(x - 0.7, x + 0.7, y - 0.35, y + 0.35, fz + 0.74, matte(0xb8b0a0, 0.4))
        R.box(x - 0.25, x + 0.25, y + 0.1, y + 0.13, fz + 0.82, fz + 1.12, matte(0x111111, 0.3))
        R.box(x - 0.05, x + 0.05, y + 0.13, y + 0.16, fz + 0.74, fz + 0.82, matte(0x111111))
        R.box(x - 0.2, x + 0.2, y - 0.75, y - 0.35, fz + 0.45, fz + 0.52, fabric(0x1a1a1e))
        R.box(x - 0.2, x + 0.2, y - 0.78, y - 0.72, fz + 0.52, fz + 1.05, fabric(0x1a1a1e))
        R.rod((x, y - 0.55, fz + 0.05), (x, y - 0.55, fz + 0.45), 0.025, matte(0x222222, 0.3, 0.8))
        R.box(x + 0.3, x + 0.6, y - 0.1, y + 0.1, fz + 0.74, fz + 0.79, matte(0xf0f0f0, 0.6))
    for i in range(4):
        x = R.x0 + 0.3 + i * 0.5
        R.box(x - 0.23, x + 0.23, d - 0.62, d, fz, fz + 1.32, matte(0x8a8a86, 0.4, 0.6), 0.008)
        for j in range(4):
            R.box(x - 0.2, x + 0.2, d - 0.625, d - 0.62, fz + 0.06 + j * 0.32, fz + 0.32 + j * 0.32, matte(0x9a9a96, 0.35, 0.6))
    R.box(R.x1 - 1.4, R.x1 - 0.2, d - 0.02, d, fz + 1.2, fz + 2.0, matte(0xf4f4f0, 0.2))
    R.door(None, 'right', y=0.8)
    R.plant(R.x1 - 0.4, d - 0.6, fz, 1.8)
    return R


def storage(cell):
    R = Room(cell, width=3.8, x_off=-0.2, height=3.0)
    R.shell(paint(0xb8b0a0, 0.95), matte(0x6a665e, 0.8), paint(0xa8a090, 0.95), crown=False)
    d, fz, cz = R.d, R.fz, R.cz
    rnd = R.rnd
    cardboard = [matte(0xa8835a, 0.9), matte(0x9a7650, 0.9), matte(0xb8946a, 0.9)]
    # Steel shelving along the back with boxes.
    st = matte(0x6a6e72, 0.4, 0.8)
    for x0 in (R.x0 + 0.1, R.x0 + 1.35):
        for z in (0.1, 0.75, 1.4, 2.05):
            R.box(x0, x0 + 1.2, d - 0.5, d - 0.05, fz + z, fz + z + 0.03, st)
            xx = x0 + 0.03
            while xx < x0 + 1.1:
                w = rnd.uniform(0.25, 0.45)
                if rnd.random() < 0.8:
                    R.box(xx, min(xx + w, x0 + 1.17), d - 0.45, d - 0.1, fz + z + 0.03, fz + z + 0.03 + rnd.uniform(0.25, 0.55), rnd.choice(cardboard), 0.005)
                xx += w + 0.03
        for x in (x0, x0 + 1.2):
            R.box(x - 0.02, x + 0.02, d - 0.5, d - 0.46, fz, fz + 2.3, st)
            R.box(x - 0.02, x + 0.02, d - 0.09, d - 0.05, fz, fz + 2.3, st)
    # Stacks on the floor, a bike, a mattress against the wall.
    for i in range(6):
        x = rnd.uniform(-0.8, 1.4)
        y = rnd.uniform(1.0, 2.2)
        h = 0.0
        for j in range(rnd.randint(1, 3)):
            w = rnd.uniform(0.35, 0.6)
            hh = rnd.uniform(0.25, 0.45)
            R.box(x - w / 2, x + w / 2, y - w / 2, y + w / 2, fz + h, fz + h + hh, rnd.choice(cardboard), 0.005)
            h += hh
    R.box(R.x1 - 0.25, R.x1 - 0.05, 0.6, 2.0, fz, fz + 1.9, fabric(0xe0dccc), 0.08)
    R.rod((0.3, 1.6, cz), (0.3, 1.6, cz - 0.4), 0.005, matte(0x111111))
    R.lathe([(0.0, 0.0), (0.03, 0.0), (0.035, 0.06), (0.0, 0.09)], (0.3, 1.6, cz - 0.49), lampshade(0xfff4d8, 30.0), 16)
    R.light((0.3, 1.6, cz - 0.45), 45.0, 0.02)
    return R


def stairwell(cell):
    R = Room(cell, width=3.4, x_off=0.4, height=3.1)
    lower, upper = paint(0x3e5a46, 0.5), paint(0xe0d8c0, 0.85)
    R.shell(upper, tiles(0x8a7a5a, 0x4a4030, 0.3, 0.5), paint(0xe8e2d0, 0.9), crown=False)
    d, fz, cz = R.d, R.fz, R.cz
    # Two-tone: the glossy dado to 1.2 m on every wall, a dark line over it.
    for a in ((R.x0, R.x1, d - 0.012, d), (R.x0, R.x0 + 0.012, 0.0, d), (R.x1 - 0.012, R.x1, 0.0, d)):
        R.box(a[0], a[1], a[2], a[3], fz, fz + 1.2, lower)
        R.box(a[0], a[1], a[2], a[3], fz + 1.2, fz + 1.24, matte(0x2a2a22, 0.4))
    # A flight rising from right to left across the back, its stringer, balusters and rail.
    steps = 12
    sx0, sx1 = R.x1 - 0.2, R.x0 + 0.4
    for i in range(steps):
        x = sx0 + (sx1 - sx0) * i / steps
        xn = sx0 + (sx1 - sx0) * (i + 1) / steps
        R.box(min(x, xn), max(x, xn), d - 1.05, d, fz, fz + 0.19 * (i + 1), wood(0x5a3a22) if i % 2 else wood(0x4e321c))
        R.box(min(x, xn), max(x, xn), d - 1.07, d - 1.05, fz + 0.19 * (i + 1) - 0.03, fz + 0.19 * (i + 1), matte(0x2a1c10, 0.5))
        bx = (x + xn) / 2
        R.rod((bx, d - 1.0, fz + 0.19 * (i + 1)), (bx, d - 1.0, fz + 0.19 * (i + 1) + 0.85), 0.012, matte(0x1a1a1a, 0.5))
    R.rod((sx0, d - 1.0, fz + 0.19 + 0.85), (sx1, d - 1.0, fz + 0.19 * steps + 0.85), 0.03, wood(0x3a2414))
    R.rod((sx0, d - 1.0, fz), (sx0, d - 1.0, fz + 1.1), 0.05, wood(0x3a2414))
    # The apartment door at the landing, its number.
    R.box(R.x0 + 0.3, R.x0 + 1.25, d - 0.02, d, fz, fz + 2.1, matte(0x6a2a1e, 0.4))
    R.box(R.x0 + 0.7, R.x0 + 0.85, d - 0.03, d - 0.02, fz + 1.55, fz + 1.65, matte(0xb8913a, 0.3, 1.0))
    R.box(R.x0 + 0.25, R.x0 + 0.3, d - 0.03, d, fz, fz + 2.15, matte(0xe8e2d0, 0.4))
    R.box(R.x0 + 1.25, R.x0 + 1.3, d - 0.03, d, fz, fz + 2.15, matte(0xe8e2d0, 0.4))
    # The caged bare bulb.
    R.lathe([(0.0, 0.0), (0.1, 0.0), (0.1, 0.02), (0.0, 0.02)], (0.2, 1.6, cz - 0.02), matte(0x222222), 16)
    R.lathe([(0.0, 0.0), (0.035, 0.0), (0.045, 0.07), (0.0, 0.11)], (0.2, 1.6, cz - 0.13), lampshade(0xfff8e0, 40.0), 16)
    for a in range(6):
        ang = a * math.pi / 3
        R.rod((0.2 + 0.07 * math.cos(ang), 1.6 + 0.07 * math.sin(ang), cz - 0.02), (0.2 + 0.07 * math.cos(ang), 1.6 + 0.07 * math.sin(ang), cz - 0.16),
              0.003, matte(0x222222), 4)
    R.light((0.2, 1.6, cz - 0.1), 70.0, 0.02)
    R.box(R.x1 - 0.012, R.x1 - 0.008, 0.6, 1.1, fz + 1.45, fz + 1.75, matte(0xd02a2a, 0.5))
    return R


def reading_room(cell):
    """The library's reading room: a tall room, shelves round it, a long table with green-shaded lamps."""
    R = Room(cell, width=5.0, x_off=0.0, height=4.6)
    R.shell(paint(0xd8cdb0), boards(0x6a4426, 0.11), paint(0xe8e0cc, 0.9))
    d, fz, cz = R.d, R.fz, R.cz
    oak = wood(0x5a3618)
    R.shelf(R.x0 + 0.2, R.x1 - 0.2, d - 0.38, d, fz, fz + 2.6, 8, mat=oak, seed=15, books=0.95)
    R.shelf(R.x0, R.x0 + 0.38, 0.4, d - 0.4, fz, fz + 2.6, 8, mat=oak, seed=16, books=0.95)
    R.shelf(R.x1 - 0.38, R.x1, 0.4, d - 0.4, fz, fz + 2.6, 8, mat=oak, seed=17, books=0.95)
    R.box(R.x0, R.x1, d - 0.4, d, fz + 2.6, fz + 2.75, oak)
    # Clerestory band and a clock over the shelves.
    R.lathe([(0.0, 0.0), (0.3, 0.0), (0.3, 0.05), (0.0, 0.05)], (0.0, 0.0, 0.0), matte(0xf4f0e4, 0.3), 32)
    R.objs[-1].data.transform(Matrix.Translation((0.0, d - 0.02, fz + 3.4)) @ Matrix.Rotation(math.pi / 2, 4, 'X'))
    R.box(-0.02, 0.02, d - 0.08, d - 0.07, fz + 3.4, fz + 3.62, matte(0x111111))
    R.box(-0.02, 0.15, d - 0.08, d - 0.07, fz + 3.39, fz + 3.42, matte(0x111111))
    # The long table, chairs, green lamps.
    R.table(-0.5, 0.5, 0.7, 2.6, fz + 0.76, oak, 0.07)
    for y in (1.1, 1.65, 2.2):
        for s in (-1, 1):
            x = s * 0.75
            R.box(x - 0.2, x + 0.2, y - 0.2, y + 0.2, fz + 0.45, fz + 0.5, oak)
            R.box(x + s * 0.17, x + s * 0.21, y - 0.2, y + 0.2, fz + 0.5, fz + 0.95, oak)
        R.lathe([(0.0, 0.0), (0.06, 0.0), (0.02, 0.03), (0.015, 0.32), (0.0, 0.32)], (0.0, y, fz + 0.76), matte(0xb8913a, 0.3, 1.0), 16)
        sh = parts.rbox('shade', (0.0, y, fz + 1.1), (0.14, 0.38, 0.1), 0.04, 3)
        core.assign(sh, lampshade(0x1e6a3a, 3.0))
        R.objs.append(sh)
        R.light((0.0, y, fz + 1.02), 22.0, 0.05)
    for x in (-1.2, 1.2):
        R.pendant(x, 1.6, 1.2, 0xf4ecd8, 60.0, 'globe')
    return R


ROOMS = [living] * 6 + [kitchen] * 3 + [bedroom] * 3 + [office, storage, stairwell, reading_room]


# ------------------------------------------------------------------ rendering

def _camera():
    sc = bpy.context.scene
    cam_data = bpy.data.cameras.new('room_cam')
    cam_data.sensor_fit = 'HORIZONTAL'
    cam_data.sensor_width = 36.0
    cam_data.lens = 18.0 / (0.5 / PIN)
    cam_data.clip_start = 0.05
    cam_data.clip_end = 50.0
    cam = core.link(bpy.data.objects.new('room_cam', cam_data))
    cam.location = (0.0, -PIN * W, H / 2)
    cam.rotation_euler = (math.pi / 2, 0.0, 0.0)
    sc.camera = cam
    sc.render.resolution_x = CELL
    sc.render.resolution_y = CELL
    sc.render.resolution_percentage = 100
    sc.render.pixel_aspect_x = 1.0
    sc.render.pixel_aspect_y = H / W
    return cam


def _render(path, samples):
    """Renders to path: .exr as linear half floats (the rooms, normalized afterwards), .png as 8-bit sRGB RGBA."""
    sc = bpy.context.scene
    sc.cycles.samples = samples
    sc.cycles.use_denoising = True
    sc.render.filepath = path
    if path.endswith('.exr'):
        sc.render.image_settings.file_format = 'OPEN_EXR'
        sc.render.image_settings.color_mode = 'RGB'
        sc.render.image_settings.color_depth = '16'
    else:
        sc.render.image_settings.file_format = 'PNG'
        sc.render.image_settings.color_mode = 'RGBA'
        sc.render.image_settings.color_depth = '8'
    bpy.ops.render.render(write_still=True)


def _lum(a):
    return a[..., 0] * 0.2126 + a[..., 1] * 0.7152 + a[..., 2] * 0.0722


def _normalize(exr, png, kind):
    """A room render (linear) to an sRGB PNG at a common exposure: by day the walls' middle tone (the median) at 0.2
    linear; a lamp pass so its bright walls (the 97th percentile, bulbs excluded) sit at 0.85."""
    a = _load(exr)[:, :, :3]
    L = _lum(a)
    if kind == 'day':
        k = 0.2 / max(float(np.median(L)), 1e-5)
    else:
        k = 0.85 / max(float(np.percentile(L, 97.0)), 1e-5)
    a = np.clip(a * k, 0.0, 1.0)
    srgb = np.where(a <= 0.0031308, a * 12.92, 1.055 * np.power(np.maximum(a, 0.0031308), 1 / 2.4) - 0.055)
    _png(png, np.dstack([srgb, np.ones(L.shape)])[::-1])


def _clear_room(R):
    for o in R.objs + R.lights:
        bpy.data.objects.remove(o, do_unlink=True)


def render_rooms(out_dir):
    sc = bpy.context.scene
    sc.view_settings.view_transform = 'Standard'
    sc.view_settings.look = 'None'
    sc.render.film_transparent = False
    world = sc.world
    world.use_nodes = True
    bg = world.node_tree.nodes.get('Background') or world.node_tree.nodes.new('ShaderNodeBackground')
    _camera()
    # Daylight: the overcast sky through the window (a broad soft source just outside it) and a cool fill.
    sky = bpy.data.lights.new('sky', 'AREA')
    sky.shape = 'RECTANGLE'
    sky.size, sky.size_y = 3.0, 3.0
    sky.color = (0.86, 0.92, 1.0)
    sky_o = core.link(bpy.data.objects.new('sky', sky))
    sky_o.location = (0.0, -0.9, H * 0.85)
    sky_o.rotation_euler = (math.radians(80), 0.0, 0.0)
    sky_o.visible_camera = False
    paths = {}
    cells = ONLY or list(range(16))
    for cell in cells:
        _LAMP_MATS.clear()
        R = ROOMS[cell](cell)
        # Day pass: lamps off, shades unlit.
        for o in R.lights:
            o.hide_render = True
        for m, glow in _LAMP_MATS:
            m.set('Emission Strength', 0.0)
        sky_o.hide_render = False
        sky.energy = 260.0 if cell != 15 else 420.0
        bg.inputs['Color'].default_value = (0.05, 0.055, 0.06, 1.0)
        sc.view_settings.exposure = 0.0
        pa = os.path.join(out_dir, f'room_{cell:02d}_day.png')
        _render(pa.replace('.png', '.exr'), 96)
        _normalize(pa.replace('.png', '.exr'), pa, 'day')
        # Lamp pass: the room's own lamps alone, 2700 K.
        for o in R.lights:
            o.hide_render = False
        for m, glow in _LAMP_MATS:
            m.set('Emission Strength', glow)
        sky_o.hide_render = True
        bg.inputs['Color'].default_value = (0.0, 0.0, 0.0, 1.0)
        pb = os.path.join(out_dir, f'room_{cell:02d}_lamp.png')
        _render(pb.replace('.png', '.exr'), 128)
        _normalize(pb.replace('.png', '.exr'), pb, 'lamp')
        paths[cell] = (pa, pb)
        _clear_room(R)
        print(f'[ns_rooms] rendered room {cell}')
    bpy.data.objects.remove(sky_o, do_unlink=True)
    return paths


# ------------------------------------------------------------------ front layers

class FrontLayer:
    """Builds one front layer in the window plane (x across, z up, y toward the room), seen from outside."""

    def __init__(self, cell):
        self.cell = cell
        self.rnd = random.Random(5100 + cell)
        self.objs = []

    def box(self, x0, x1, y0, y1, z0, z1, mat, r=0.0):
        o = parts.rbox('b', ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), r, 2)
        core.assign(o, mat)
        self.objs.append(o)
        return o

    def drape(self, x0, x1, z0, z1, mat, pitch=0.07, amp=0.025, gather=1.0, y=0.05, tie=None):
        """A pleated panel from x0 to x1 hanging from z1 to z0, gathered toward one end if tie (z, x) is given."""
        import bmesh
        nu = max(4, int(abs(x1 - x0) / (pitch / 2)))
        nv = 24
        bm = bmesh.new()
        grid = []
        for j in range(nv + 1):
            t = j / nv
            z = z1 + (z0 - z1) * t
            row = []
            for i in range(nu + 1):
                s = i / nu
                x = x0 + (x1 - x0) * s
                if tie is not None:
                    tz, tx = tie
                    pull = math.exp(-((z - tz) / 0.35) ** 2) * 0.7 + (0.0 if z > tz else 0.25)
                    x = x + (tx - x) * pull * gather
                dy = amp * (1 if i % 2 else -1) * (0.6 + 0.4 * math.sin(t * 3.0 + i))
                row.append(bm.verts.new((x, y + dy, z)))
            grid.append(row)
        for j in range(nv):
            for i in range(nu):
                bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
        o = core.mesh_object('drape', bm)
        o.data.shade_smooth()
        core.assign(o, mat)
        self.objs.append(o)
        return o

    def slats(self, z0, z1, mat, tilt=0.6, pitch=0.025, x0=-W / 2 + 0.03, x1=W / 2 - 0.03):
        z = z1 - 0.04
        self.box(x0 - 0.01, x1 + 0.01, 0.02, 0.06, z1 - 0.04, z1, mat, 0.005)
        while z > z0:
            o = parts.rbox('slat', (0.0, 0.0, 0.0), (x1 - x0, 0.025, 0.002), 0.0, 1)
            o.data.transform(Matrix.Translation(((x0 + x1) / 2, 0.04, z)) @ Matrix.Rotation(tilt, 4, 'X'))
            core.assign(o, mat)
            self.objs.append(o)
            z -= pitch
        self.box(x0, x1, 0.03, 0.05, z0 - 0.02, z0, mat, 0.004)
        for x in (x0 + 0.15, x1 - 0.15):
            self.box(x - 0.002, x + 0.002, 0.039, 0.041, z0, z1, matte(0xd8d2c0, 0.8))


def front_layers():
    out = []
    for cell in range(16):
        F = FrontLayer(cell)
        if cell in (0, 1, 11):
            h = (0x8a3a22, 0x2a6a6a, 0x6a4a7a)[cell if cell < 2 else 2]
            f = fabric(h)
            F.box(-W / 2 - 0.05, W / 2 + 0.05, 0.06, 0.08, H - 0.05, H - 0.03, matte(0x3a2a1a, 0.4))
            F.drape(-W / 2 - 0.02, -W / 2 + 0.38, -0.02, H - 0.05, f, tie=(0.8, -W / 2 + 0.05))
            F.drape(W / 2 - 0.38, W / 2 + 0.02, -0.02, H - 0.05, f, tie=(0.8, W / 2 - 0.05))
        elif cell == 2:
            F.drape(-W / 2, W / 2, 0.0, H - 0.03, sheer(0xf0ece0, 0.45), pitch=0.05, amp=0.012)
        elif cell in (3, 4, 12):
            z0 = {3: H * 0.5, 4: 0.02, 12: 0.02}[cell]
            F.slats(z0, H - 0.02, matte(0xd8d0bc, 0.5), tilt={3: 1.2, 4: 0.9, 12: 1.5}[cell])
        elif cell == 5:
            F.box(-W / 2, W / 2, 0.02, 0.03, H * 0.66, H - 0.04, fabric(0xd8cdb0, 0xc8bda0))
            F.box(-W / 2 + 0.02, W / 2 - 0.02, 0.0, 0.03, H * 0.66 - 0.03, H * 0.66, matte(0x6a5a40, 0.4))
            F.box(-0.04, 0.04, 0.0, 0.01, H * 0.66 - 0.12, H * 0.66 - 0.03, matte(0xd8d0b8))
        elif cell == 13:
            # A blanket tacked up across the window: a camp blanket, its stripes near the ends, sagging between tacks.
            F.drape(-W / 2 + 0.02, W / 2 - 0.02, 0.05, H - 0.04, fabric(0x8a7458, 0x6e5c44), pitch=0.3, amp=0.02)
            for z in (0.25, 0.35, H - 0.45, H - 0.35):
                F.box(-W / 2 + 0.02, W / 2 - 0.02, 0.0, 0.03, z, z + 0.05, fabric(0x2a2a3a))
        elif cell == 14:
            news = Sheet(1000, 1500)
            news.rect(0, 0, 1000, 1500, 0xd8d2c0, rough=0.9)
            rnd = random.Random(14)
            for c in range(4):
                for r in range(60):
                    if rnd.random() < 0.85:
                        news.rect(30 + c * 240, 1450 - r * 23, rnd.uniform(140, 215), 9, 0x3a3a38, rough=0.9)
            news.text('THE NORTHSIDE CALL', 500, 1380, 70, 0x1a1a1a, face='Black', align='CENTER')
            col, _ = news.render('ns_roomfront_news', 512)
            m = core.Mat('news_paper')
            img = m.image(col)
            m.link(img.outputs['Color'], m.bsdf.inputs['Base Color'])
            m.set('Roughness', 0.9)
            o = F.box(-W / 2 + 0.03, W / 2 - 0.03, 0.0, 0.004, 0.03, H - 0.03, m)
            core.uv_layout(o, [(lambda f: True, 'planar', ('x', 'z', -W / 2 + 0.03, W / 2 - 0.03, 0.03, H - 0.03), (0.0, 0.0, 1.0, 1.0))])
        elif cell == 9:
            sign = Sheet(600, 400)
            sign.rect(0, 0, 600, 400, 0xf0ece0, rough=0.8)
            sign.rect(0, 300, 600, 100, 0xc8202c, rough=0.8)
            sign.text('FOR RENT', 300, 322, 72, 0xffffff, face='Black', align='CENTER')
            sign.text('2 BR', 300, 190, 70, 0x1a1a1a, face='Black', align='CENTER')
            sign.text('555-0119', 300, 60, 64, 0xc8202c, face='Black', align='CENTER')
            col, _ = sign.render('ns_roomfront_forrent', 512)
            m = core.Mat('forrent')
            img = m.image(col)
            m.link(img.outputs['Color'], m.bsdf.inputs['Base Color'])
            m.set('Roughness', 0.8)
            o = F.box(-0.3, 0.3, 0.0, 0.002, H * 0.55, H * 0.55 + 0.4, m)
            core.uv_layout(o, [(lambda f: True, 'planar', ('x', 'z', -0.3, 0.3, H * 0.55, H * 0.55 + 0.4), (0.0, 0.0, 1.0, 1.0))])
            for x in (-0.28, 0.28):
                F.box(x - 0.04, x + 0.04, -0.001, 0.0, H * 0.55 + 0.33, H * 0.55 + 0.42, matte(0xe0d8b0, 0.3))
        elif cell == 10:
            F.box(-W / 2, W / 2, 0.06, 0.07, H * 0.48 - 0.01, H * 0.48 + 0.01, matte(0xb8b8b0, 0.3, 1.0))
            F.drape(-W / 2, W / 2, 0.0, H * 0.48, fabric(0xe8dcc0, 0xd8c8a0), pitch=0.05, amp=0.015)
        if cell in (6, 7, 11):
            R = Room(99)
            R.fz = 0.0
            R.objs = F.objs
            R.plant(-0.25 if cell != 7 else -0.3, 0.12, 0.0, 0.9)
            if cell == 7:
                R.plant(0.3, 0.12, 0.0, 0.7)
                R.lathe([(0.0, 0.0), (0.05, 0.0), (0.05, 0.14), (0.03, 0.16), (0.0, 0.16)], (0.05, 0.1, 0.0), matte(0x6a8a9a, 0.1), 16)
        if cell == 8:
            # A cat on the sill, sitting, seen from behind and a little from the side (dark: lit from the room).
            cm = matte(0x1a1612, 0.8)
            o = parts.rbox('body', (0.12, 0.12, 0.17), (0.2, 0.18, 0.32), 0.09, 4)
            core.assign(o, cm)
            F.objs.append(o)
            o = parts.rbox('head', (0.12, 0.1, 0.38), (0.13, 0.11, 0.11), 0.05, 4)
            core.assign(o, cm)
            F.objs.append(o)
            for s in (-1, 1):
                o = parts.rbox('ear', (0.12 + s * 0.04, 0.1, 0.45), (0.04, 0.02, 0.05), 0.008, 2)
                core.assign(o, cm)
                F.objs.append(o)
            F.objs.append(parts.tube('tail', [(0.2, 0.15, 0.02), (0.28, 0.12, 0.04), (0.33, 0.08, 0.02)], 0.018))
            core.assign(F.objs[-1], cm)
        out.append(F)
    return out


def render_fronts(out_dir):
    sc = bpy.context.scene
    layers = front_layers()          # first: the printed ones render sheets, which take over the camera and resolution
    cam = bpy.data.objects.get('room_cam') or _camera()
    sc.camera = cam
    sc.render.resolution_x = CELL
    sc.render.resolution_y = CELL
    sc.render.pixel_aspect_x = 1.0
    sc.render.pixel_aspect_y = H / W
    sc.render.film_transparent = True
    sc.view_settings.view_transform = 'Standard'
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = W
    cam.location = (0.0, -3.0, H / 2)
    key = bpy.data.lights.new('front_key', 'AREA')
    key.size = 4.0
    key.energy = 110.0
    key_o = core.link(bpy.data.objects.new('front_key', key))
    key_o.location = (0.5, -2.5, H)
    key_o.rotation_euler = (math.radians(70), 0.0, math.radians(10))
    key_o.visible_camera = False
    world = sc.world
    bg = world.node_tree.nodes.get('Background')
    bg.inputs['Color'].default_value = (0.16, 0.165, 0.175, 1.0)
    paths = {}
    for F in layers:
        for o in F.objs:
            o.hide_render = True
    for F in layers:
        for o in F.objs:
            o.hide_render = False
        p = os.path.join(out_dir, f'front_{F.cell:02d}.png')
        if F.objs:
            _render(p, 64)
        else:
            sc.render.filepath = p
            _render(p, 1)
        paths[F.cell] = p
        for o in F.objs:
            bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.objects.remove(key_o, do_unlink=True)
    cam.data.type = 'PERSP'
    cam.location = (0.0, -PIN * W, H / 2)
    sc.render.film_transparent = False
    return paths


# ------------------------------------------------------------------ the atlases

def _load(path):
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    a = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return a.reshape(h, w, 4)    # rows bottom-up


def _png(path, arr):
    """Writes an H x W x C float array (rows top-down) as an 8-bit PNG exactly as given."""
    import struct
    import zlib
    a = np.asarray(arr, dtype=np.float32)
    h, w, c = a.shape
    data = (np.clip(a, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)
    raw = b''.join(b'\x00' + data[y].tobytes() for y in range(h))

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body) & 0xffffffff)
    kind = {3: 2, 4: 6}[c]
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, kind, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b'')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(png)


def _atlas(name, cell_paths, channels=3):
    """Assembles 16 cell renders into a 4 x 4 atlas (cell i at column i % 4, row i // 4 from the top); cells not
    rendered this time come from the atlas already on disk."""
    path = os.path.join(TEX_DIR, f'{name}.png')
    N = CELL
    out = np.zeros((4 * N, 4 * N, channels), np.float32)
    if len(cell_paths) < 16 and os.path.exists(path):
        old = _load(path)[::-1, :, :channels]
        if old.shape[0] == 4 * N:
            out[:] = old
    for cell, p in cell_paths.items():
        a = _load(p)[::-1]          # rows top-down
        col, row = cell % 4, cell // 4
        out[row * N:(row + 1) * N, col * N:(col + 1) * N] = a[:, :, :channels]
    _png(path, out)
    print(f'[ns_rooms] wrote {os.path.relpath(path, core.ROOT)} ({4 * N}^2)')
    return path


def build():
    core.reset()
    sc = bpy.context.scene
    sc.cycles.device = core.cycles_device()
    out_dir = os.path.join(core.BUILD_DIR, 'textures', 'ns_rooms')
    os.makedirs(out_dir, exist_ok=True)
    rooms = render_rooms(out_dir)
    albedo = _atlas('T_NS_Rooms_Albedo', {c: p[0] for c, p in rooms.items()})
    lamp = _atlas('T_NS_Rooms_Lamp', {c: p[1] for c, p in rooms.items()})
    fronts = _atlas('T_NS_RoomFront', render_fronts(out_dir), channels=4)
    # Review cards (never exported): the three atlases side by side.
    _CARDS.clear()
    for i, (p, label) in enumerate(((albedo, 'albedo'), (fronts, 'fronts'), (lamp, 'lamp'))):
        bpy.ops.mesh.primitive_plane_add(size=5.0, location=((i - 1) * 6.0, 0.0, 0.0), rotation=(math.pi / 2, 0.0, 0.0))
        o = bpy.context.view_layer.objects.active
        o.name = f'card_{label}'
        m = core.Mat(f'card_{label}')
        img = m.image(p)
        em = m.node('ShaderNodeEmission', Strength=1.0)
        m.link(img.outputs['Color'], em.inputs['Color'])
        m.link(em.outputs['Emission'], m.out.inputs['Surface'])
        core.assign(o, m)
        _CARDS.append(o)
    return list(_CARDS)


def assemble(objs):
    """Nothing to export: the atlases are the product."""
    return []


def pose_for_review(objs):
    """art/review/ns_rooms_cells.jpg: every room, day | lamp, and every front layer, readable; then the standard
    sheet shows the three atlas cards."""
    d = os.path.join(core.BUILD_DIR, 'textures', 'ns_rooms')
    tiles = []
    for cell in range(16):
        a, b = os.path.join(d, f'room_{cell:02d}_day.png'), os.path.join(d, f'room_{cell:02d}_lamp.png')
        if os.path.exists(a) and os.path.exists(b):
            tiles.append((_load(a), _load(b)))
    if tiles:
        n = tiles[0][0].shape[0]
        hcell = int(n * H / W) // 2
        wcell = n // 2

        def shrink(im):
            # Stretch back to the opening's aspect (1:1.5) at half size.
            ys = (np.arange(hcell) * im.shape[0] / hcell).astype(int)
            xs = (np.arange(wcell) * im.shape[1] / wcell).astype(int)
            return im[ys][:, xs, :3]
        gap = 6
        cols = 8
        rows = 4
        Wt = cols * wcell + (cols - 1) * gap
        Ht = rows * hcell + (rows - 1) * gap
        out = np.full((Ht, Wt, 3), 0.05, np.float32)
        for i, (a, b) in enumerate(tiles):
            r, c = i // 4, (i % 4) * 2
            for k, im in enumerate((a, b)):
                y0 = Ht - (r + 1) * hcell - r * gap
                x0 = (c + k) * (wcell + gap)
                out[y0:y0 + hcell, x0:x0 + wcell] = shrink(im)
        img = bpy.data.images.new('rv_rooms', Wt, Ht, alpha=False)
        img.pixels.foreach_set(np.dstack([out, np.ones((Ht, Wt))]).astype(np.float32).ravel())
        path = os.path.join(core.REVIEW_DIR, 'ns_rooms_cells.jpg')
        img.filepath_raw = path
        img.file_format = 'JPEG'
        img.save(quality=90)
        bpy.data.images.remove(img)
        print(f'[ns_rooms] review sheet {os.path.relpath(path, core.ROOT)}')
        # The front layers over a lit room (cell 0's lamp pass), as they'd hang in a lit window.
        backdrop = tiles[0][1]
        out = np.full((2 * hcell + gap, Wt, 3), 0.05, np.float32)
        for cell in range(16):
            p = os.path.join(d, f'front_{cell:02d}.png')
            if not os.path.exists(p):
                continue
            f = _load(p)
            a = f[:, :, 3:4]
            im = shrink(np.dstack([f[:, :, :3] * a[:, :, 0:1] + backdrop[:, :, :3] * (1 - a), np.ones(a.shape)]))
            r, c = cell // 8, cell % 8
            y0 = out.shape[0] - (r + 1) * hcell - r * gap
            x0 = c * (wcell + gap)
            out[y0:y0 + hcell, x0:x0 + wcell] = im
        img = bpy.data.images.new('rv_fronts', Wt, out.shape[0], alpha=False)
        img.pixels.foreach_set(np.dstack([out, np.ones(out.shape[:2])]).astype(np.float32).ravel())
        path = os.path.join(core.REVIEW_DIR, 'ns_rooms_fronts.jpg')
        img.filepath_raw = path
        img.file_format = 'JPEG'
        img.save(quality=90)
        bpy.data.images.remove(img)
        print(f'[ns_rooms] review sheet {os.path.relpath(path, core.ROOT)}')
    objs.extend(_CARDS)
