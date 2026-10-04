"""SM_Facade_*: the building fronts along Fifth Street, Northside, on a rainy night.

AStreetStage (unreal/Source/ShortStack/StreetStage.cpp) blocks the street out of boxes with flat window slabs. These
are the fronts that dress it, made the way SM_Tenement is (tenement.py, the building the apartment looks out on):
masonry with real reveals, double-hung sashes, stone sills and lintels, a room behind every lit window with its own
lamp lighting it, ironwork, cornices and signage. Four meshes, one building front each:

- SM_Facade_Walkup: a five-storey red-brick walk-up (common bond, a pressed-metal cornice) over a cast-iron
  shopfront: Fifth St. Barbers on the left, shut for the night behind its roll-down grille with the night light on
  and the pole lit; the tenants' door in the middle, in a marble-lined recess; Northside Tailor & Cleaners on the
  right, its gate down, tagged and wheat-pasted. A fire escape down the right two bays with a FOR RENT banner.
- SM_Facade_Brownstone: a brownstone row house: a stoop with cast-iron rails up to the parlor floor's double doors,
  a three-sided bay from the ground to the cornice, carved hoods over the windows, an iron-fenced areaway, a
  bracketed wood cornice. Two side by side are one walk-up wide.
- SM_Facade_1812: the player's building. Buff brick on a rusticated limestone base; the entrance recessed in a
  stone surround: door 1812 with the hall light through its glass and the brass mailboxes inside, the buzzer panel
  on the recess wall, the number in brass on the frieze and in gold leaf on the lit transom, a carriage lamp each
  side (low and wide of the number, so it never blooms over it) and a downlight in the recess. The player's window
  (third floor, Y +3.2: the bay left of the entrance as you face it, as in StreetStage) glows with the laptop.
- SM_Facade_StoreUpper: the three floors over the Lucky Penny at the corner of Market Street, a closed block that
  sits on the store's roof: painted brick with stone quoins, the corner's return down Market (+Y) with its fire
  escape and a faded painted ad (ROYAL FLUSH CIGARS), an income-tax office's gold leaf on the second floor, a water
  tank on the roof.

Coordinates (meters), as SM_Tenement: origin at street level (the sidewalk's top) on the front's face, Z up, the
front facing -X, Y along the street (+Y on the left of someone facing the front), centered on Y. What stands proud
of the face (stoops, sills, cornices, fire escapes, signs) is at -X; the rooms behind the lit windows reach back to
ROOM_BACK (3.6 m), where each front is closed by a plain back, so any massing behind a front has to start that far
back. The glass in front of lit rooms and shop windows is left out (the bake has no transmission), as in
SM_Tenement. Placed like SM_Tenement (no BlenderFacing): yaw 0 faces -X (the far side of Fifth), yaw 180 faces +X
(the home side, where Blender +Y lands on the stage's +Y).

Each mesh has five materials, in this slot order: 0 the wall (brick or brownstone), 1 a trim atlas (stone, paint,
iron, brass, tile... chosen per face by an attribute and baked into one texture set), 2 the dark window glass,
3 the lit interiors (emissive: rooms, shops, lamps: the slot to dim by day), 4 the prints (signs, numbers, labels;
emissive where a sign is lit or backlit). A baked texture that comes out one flat value (the lit slot's base color,
ORM and normal; the prints' and some glass's normal) is exported as a 4x4 (assemble()).

Faces of separate parts never lie in one plane facing the same way (it z-fights in the game): stone heads and jambs
reach 3 mm into their openings, gratings sit under their frames, party-wall returns start behind the trim's ends.

FACADE_ONLY=walkup,1812 (environment) builds just those fronts, for iterating.
"""
import math
import os
import random
import sys

import bmesh
import numpy as np
import bpy  # noqa: F401
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import convex_edges, image_surface, mix_float, noise, object_coords, ramp, scaled, smooth_less, vec
from artkit.sheet import Sheet

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import brand  # noqa: E402

MESH = {'walkup': 'SM_Facade_Walkup', 'brownstone': 'SM_Facade_Brownstone', '1812': 'SM_Facade_1812', 'storeupper': 'SM_Facade_StoreUpper'}
PREFIX = {'walkup': 'wu', 'brownstone': 'bs', '1812': 'ft', 'storeupper': 'su'}
_ONLY = [s.strip() for s in os.environ.get('FACADE_ONLY', '').split(',') if s.strip()]
WHICH = [k for k in MESH if not _ONLY or k in _ONLY]
MESHES = [MESH[k] for k in WHICH]
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {}
for _p in PREFIX.values():
    TEXTURE_SIZES.update({f'{_p}_wall': 2048, f'{_p}_trim': 2048, f'{_p}_glass': 512, f'{_p}_lit': 2048, f'{_p}_print': 1024})
TEXTURE_SIZES.update({'bs_lit': 1024, 'bs_print': 512, 'ft_wall': (4096, 2048), 'su_wall': (4096, 2048), 'su_lit': 1024, 'su_print': 512})
AO_DISTANCE = 0.45
REVIEW_SCREEN_LIGHT = False
REVIEW_VIEWS = None  # set by pose_for_review, once the fronts' bounds are known

ROOM_BACK = 3.6   # nothing of a front goes further back into the block than this
RD = 1.9          # how deep a lit room runs behind its window's reveal

# Trim kinds (the trim atlas's ss_kind) and lit kinds (the interiors' ss_kind).
TK = {n: i + 1 for i, n in enumerate(['stone', 'sash', 'front', 'door', 'iron', 'galv', 'brass', 'alu', 'ac', 'tar', 'tile', 'gate', 'granite', 'wood',
                                      'black', 'brown', 'cornice', 'concrete', 'plywood', 'grate', 'casing', 'steel', 'marble', 'pot', 'leaf', 'enamel', 'cedar'])}
IK = {n: i + 1 for i, n in enumerate(['wall', 'ceil', 'floor', 'curtain', 'blinds', 'shade', 'fixture', 'furn', 'screen', 'frame', 'plant', 'checker',
                                      'subway', 'mirror', 'vinyl', 'chrome', 'counter', 'hex', 'marble', 'mailbox', 'oak', 'glow', 'pole', 'lantern',
                                      'poster', 'cabinet', 'jar', 'stair'])}
# A lit room's light (ss_room.x) and its color.
LOOKS = {'warm': 1, 'cool': 2, 'tv': 3, 'laptop': 4, 'red': 5, 'shop': 6, 'hall': 7, 'stair': 7, 'drawn': 1, 'blinds': 1}
LOOK_HEX = {1: 0xffaf62, 2: 0xe2ecff, 3: 0x5f86ff, 4: 0x9dbcff, 5: 0xff4a34, 6: 0xffe2c0, 7: 0xffc98a}
ROOM_LOOKS = ('warm', 'cool', 'tv', 'laptop', 'red', 'stair')
# A dark window's look (ss_win2.z).
GLASS = {'dark': 0, 'shade': 1, 'sheer': 2, 'vblind': 3, 'foil': 4, 'paper': 5}


def hexc(h):
    return core.hex_linear(h)


def _shade(h, f):
    r, g, b = (h >> 16) & 255, (h >> 8) & 255, h & 255
    r, g, b = (max(0, min(255, int(c * f))) for c in (r, g, b))
    return (r << 16) | (g << 8) | b


def _tint(h, f):
    r, g, b = (h >> 16) & 255, (h >> 8) & 255, h & 255
    r, g, b = (int(c + (255 - c) * f) for c in (r, g, b))
    return (r << 16) | (g << 8) | b


# ------------------------------------------------------------------ attributes and shader helpers

def set_attr(obj, name, value):
    """A per-face attribute on every face of obj (a float, or a 3-vector), read by the shaders."""
    me = obj.data
    n = len(me.polygons)
    is_vec = isinstance(value, (tuple, list, Vector))
    a = me.attributes.get(name)
    if a is None:
        a = me.attributes.new(name, 'FLOAT_VECTOR' if is_vec else 'FLOAT', 'FACE')
    if is_vec:
        v = tuple(value)
        a.data.foreach_set('vector', [float(v[i % 3]) for i in range(n * 3)])
    else:
        a.data.foreach_set('value', [float(value)] * n)


def attr(m, name, out='Fac'):
    n = m.node('ShaderNodeAttribute')
    n.attribute_type = 'GEOMETRY'
    n.attribute_name = name
    return n.outputs[out]


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


def split(m, v):
    n = m.node('ShaderNodeSeparateXYZ')
    m.link(v, n.inputs['Vector'])
    return n.outputs['X'], n.outputs['Y'], n.outputs['Z']


def is_k(m, s, k):
    return m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', s, float(k))), 0.5)


def cmul(m, col, s):
    """A color times a scalar (a constant or a socket)."""
    if isinstance(s, (int, float)):
        return m.mix(1.0, col, (s, s, s, 1.0), blend='MULTIPLY')
    return m.mix(1.0, col, vec(m, s, s, s), blend='MULTIPLY')


def ctimes(m, a, b):
    return m.mix(1.0, a, b, blend='MULTIPLY')


def white_noise(m, v):
    n = m.node('ShaderNodeTexWhiteNoise')
    n.noise_dimensions = '3D'
    m.link(v, n.inputs['Vector'])
    return n.outputs['Value'], n.outputs['Color']


def face_uv(m, P, N):
    """(u, v) on a face: u along it, left to right as you face it (the in-plane horizontal t = (-N.y, N.x, 0)), v up;
    horizontal faces take x and y."""
    nx, ny, nz = split(m, N)
    t = vmath(m, 'NORMALIZE', vec(m, m.math('MULTIPLY', ny, -1.0), nx, 0.0))
    u_wall = vmath(m, 'DOT_PRODUCT', P, t)
    x, y, z = split(m, P)
    flat = ramp(m, m.math('ABSOLUTE', nz), 0.6, 0.8)
    return mix_float(m, flat, u_wall, x), mix_float(m, flat, z, y)


def face_t(n):
    """The in-plane horizontal of a face with normal n (the u axis the shaders use)."""
    n = Vector(n)
    t = Vector((-n.y, n.x, 0.0))
    return t.normalized() if t.length > 1e-6 else Vector((0.0, -1.0, 0.0))


class Ctx:
    """The sockets every weathered surface starts from (object space)."""

    def __init__(self, m, edges=False):
        tc, sep = object_coords(m)
        self.tc, self.sep = tc, sep
        self.P = tc.outputs['Object']
        self.x, self.y, self.z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
        self.N = m.node('ShaderNodeNewGeometry').outputs['Normal']
        self.u, self.v = face_uv(m, self.P, self.N)
        self.mottle = noise(m, self.P, 1.3, 4.0)
        self.streak = noise(m, scaled(m, sep, 7.0, 7.0, 0.35), 1.0, 3.0)
        self.wet = ramp(m, self.streak, 0.5, 0.72)
        self.grime = ramp(m, self.z, 1.4, 0.0)
        self.edges = convex_edges(m, tc, distance=0.008, samples=12, breakup_scale=22.0, gain=5.0) if edges else None


def weather(m, c, col, soot=0.5, grime=0.6, wet=0.35):
    """Rain streaks of soot running down, road dirt splashed up from the sidewalk, and darker where it's wet."""
    col = m.mix(m.math('MULTIPLY', ramp(m, c.streak, 0.52, 0.74), soot), col, hexc(0x1b140f))
    col = m.mix(m.math('MULTIPLY', c.grime, grime), col, hexc(0x231b15))
    col = m.mix(m.math('MULTIPLY', c.wet, wet), col, cmul(m, col, 0.6))
    return col


def bump(m, height, strength=0.5, distance=0.004):
    b = m.node('ShaderNodeBump', Strength=strength, Distance=distance)
    m.link(height, b.inputs['Height'])
    m.set('Normal', b.outputs['Normal'])


# ------------------------------------------------------------------ the wall materials

def brick_material(name, c1, c2, mortar, light, dark, soot=0.55, bond='running', paint=None, ghost=None):
    """Brick (21.5 x 6.8 cm modules, 1 cm joints) laid running or common bond (a header course every sixth), per-brick
    color with clinkers and washed-out ones; optionally painted over and peeling; optionally a faded painted ad.
    Soot streaks from the rain, road grime, darker and glossier where it's wet."""
    m = core.Mat(name)
    c = Ctx(m)
    face = vec(m, c.u, c.v, 0.0)
    BW, RH = 0.215, 0.068

    def tex(width, col1, col2, mort):
        b = m.node('ShaderNodeTexBrick', Scale=1.0, **{'Mortar Size': 0.0095, 'Mortar Smooth': 0.25, 'Bias': 0.0, 'Brick Width': width, 'Row Height': RH})
        b.offset = 0.5
        b.offset_frequency = 2
        b.squash = 1.0
        b.squash_frequency = 2
        b.inputs['Color1'].default_value = col1
        b.inputs['Color2'].default_value = col2
        b.inputs['Mortar'].default_value = mort
        m.link(face, b.inputs['Vector'])
        return b

    k0, k1 = (0.0, 0.0, 0.0, 1.0), (1.0, 1.0, 1.0, 1.0)
    # The brick texture gives the joints; each brick's own randoms come from a white noise of its cell (the
    # texture's built-in tint is correlated along diagonals, which shows as chevrons across a whole wall).
    row = m.math('FLOOR', m.math('DIVIDE', c.v, RH))
    even = m.math('SUBTRACT', 1.0, m.math('FLOORED_MODULO', row, 2.0))

    def cell(width):
        num = m.math('FLOOR', m.math('DIVIDE', m.math('ADD', c.u, m.math('MULTIPLY', even, width * 0.5)), width))
        v, colr = white_noise(m, vec(m, num, row, width * 7.3))
        return v, split(m, colr)[0]

    mort = tex(BW, k0, k1, k0).outputs['Fac']
    rb, rt = cell(BW)
    if bond == 'common':
        hdr = m.math('LESS_THAN', m.math('FLOORED_MODULO', row, 6.0), 0.5)
        mort = mix_float(m, hdr, mort, tex(BW / 2, k0, k1, k0).outputs['Fac'])
        hb, ht = cell(BW / 2)
        rb, rt = mix_float(m, hdr, rb, hb), mix_float(m, hdr, rt, ht)
    col = m.mix(mort, m.mix(rt, hexc(c1), hexc(c2)), hexc(mortar))
    # Brick by brick: dark clinkers, washed-out ones, a burnt end here and there.
    col = m.mix(m.math('MULTIPLY', ramp(m, rb, 0.925, 0.945), 0.8), col, hexc(dark))
    col = m.mix(m.math('MULTIPLY', ramp(m, rb, 0.09, 0.04), 0.65), col, hexc(light))
    speck = noise(m, c.P, 38.0, 2.0)
    col = m.mix(m.math('MULTIPLY', ramp(m, speck, 0.55, 0.8), 0.35), col, hexc(_shade(c2, 0.6)))
    mottle = noise(m, c.P, 0.35, 3.0)
    col = m.mix(m.math('MULTIPLY', ramp(m, mottle, 0.45, 0.8), 0.3), col, hexc(light))
    # Salt bloom low down and under the cornice.
    bloom = m.math('MULTIPLY', ramp(m, noise(m, c.P, 2.2, 4.0), 0.58, 0.72), ramp(m, c.z, 1.6, 0.3))
    col = m.mix(m.math('MULTIPLY', bloom, 0.3), col, hexc(0xb9b3a6))
    rough = m.math('ADD', m.math('MULTIPLY', mort, 0.22), 0.66)
    height = m.math('ADD', m.math('SUBTRACT', 1.0, mort), m.math('MULTIPLY', speck, 0.15))
    if paint:
        # Paint over the brick, letting go in patches a meter or two across (more where the rain runs), ragged at
        # the edges: the top coat lifts first, showing an older cream coat in a band round the bare brick, the
        # lifted edge catching the light. The coats are thin: the brick's joints and its color show through a little.
        phex, peel = paint
        big = noise(m, c.P, 0.42, 3.0, 0.55)
        rag = noise(m, c.P, 5.0, 8.0, 0.72)
        where = m.math('ADD', m.math('MULTIPLY', ramp(m, c.streak, 0.5, 0.75), 0.6), m.math('MULTIPLY', ramp(m, noise(m, c.P, 0.15, 2.0), 0.45, 0.7), 0.4))
        pn = m.math('ADD', m.math('ADD', m.math('MULTIPLY', big, 0.74), m.math('MULTIPLY', rag, 0.26)), m.math('MULTIPLY', where, 0.07 * peel))
        t = 0.627 - 0.05 * peel
        gone = ramp(m, pn, t, t + 0.004)
        coat = m.math('SUBTRACT', ramp(m, pn, t - 0.014, t - 0.011), gone)
        lip = m.math('SUBTRACT', ramp(m, pn, t - 0.018, t - 0.015), ramp(m, pn, t - 0.014, t - 0.011))
        pcol = m.mix(m.math('MULTIPLY', ramp(m, c.mottle, 0.3, 0.8), 0.45), hexc(phex), hexc(_shade(phex, 0.78)))
        # Touched up over the years: broad areas a shade lighter or darker than the rest.
        pcol = m.mix(m.math('MULTIPLY', ramp(m, noise(m, c.P, 0.22, 2.0), 0.35, 0.7), 0.5), pcol, hexc(_tint(phex, 0.12)))
        pcol = m.mix(0.07, pcol, col)
        pcol = m.mix(m.math('MULTIPLY', mort, 0.22), pcol, hexc(_shade(phex, 0.62)))
        pcol = m.mix(m.math('MULTIPLY', lip, 0.7), pcol, hexc(_tint(phex, 0.35)))
        old = m.mix(m.math('MULTIPLY', mort, 0.25), hexc(0xb3a688), hexc(0x7d7360))
        pcol = m.mix(coat, pcol, old)
        col = m.mix(gone, pcol, m.mix(ramp(m, pn, t + 0.004, t + 0.012), cmul(m, col, 0.55), col))
        rough = mix_float(m, gone, mix_float(m, coat, 0.6, 0.72), rough)
        # The paint fills the joints half way and takes the same sheen over brick and mortar alike.
        height = mix_float(m, gone, m.math('SUBTRACT', height, m.math('MULTIPLY', m.math('SUBTRACT', 1.0, mort), 0.5)), height)
        height = m.math('ADD', height, m.math('ADD', m.math('MULTIPLY', gone, -0.4), m.math('MULTIPLY', coat, -0.15)))
    # Old brick faces aren't flat or alike: each brick its own sheen, the faces pitted and dished a little, so a
    # wet wall glints brick by brick instead of shining like tile.
    pits = noise(m, c.P, 14.0, 4.0)
    height = m.math('ADD', height, m.math('MULTIPLY', pits, 0.22))
    rough = m.math('ADD', rough, m.math('ADD', m.math('MULTIPLY', m.math('SUBTRACT', rt, 0.5), 0.14), m.math('MULTIPLY', m.math('SUBTRACT', pits, 0.5), 0.16)))
    if ghost:
        gu = m.math('DIVIDE', m.math('SUBTRACT', c.u, ghost['a0']), ghost['a1'] - ghost['a0'])
        gv = m.math('DIVIDE', m.math('SUBTRACT', c.z, ghost['z0']), ghost['z1'] - ghost['z0'])
        ink, alpha, _, _ = image_surface(m, ghost['paths'], vec(m, gu, gv, 0.0))
        nv = vmath(m, 'DOT_PRODUCT', c.N, tuple(ghost['n']))
        inside = m.math('MULTIPLY', m.math('MULTIPLY', m.math('GREATER_THAN', gu, 0.0), m.math('LESS_THAN', gu, 1.0)),
                        m.math('MULTIPLY', m.math('GREATER_THAN', gv, 0.0), m.math('LESS_THAN', gv, 1.0)))
        inside = m.math('MULTIPLY', inside, m.math('GREATER_THAN', nv, 0.9))
        wear = ramp(m, noise(m, c.P, 2.6, 5.0, 0.6), 0.3, 0.62)
        f = m.math('MULTIPLY', m.math('MULTIPLY', inside, alpha), m.math('MULTIPLY', wear, ghost.get('fade', 0.6)))
        f = m.math('MULTIPLY', f, m.math('SUBTRACT', 1.0, m.math('MULTIPLY', mort, 0.6)))
        col = m.mix(f, col, ink)
    # Soot streaks: broad runs and thin ones, clustered (under a failed gutter, down from the sills) rather than
    # combed evenly down the whole wall.
    streak = noise(m, scaled(m, c.sep, 3.2, 3.2, 0.12), 1.0, 3.0)
    thin = noise(m, scaled(m, c.sep, 11.0, 11.0, 0.3), 1.0, 3.0)
    cluster = ramp(m, noise(m, scaled(m, c.sep, 0.35, 0.35, 0.22), 1.0, 2.0), 0.42, 0.6)
    runs = m.math('MAXIMUM', ramp(m, streak, 0.53, 0.74), m.math('MULTIPLY', ramp(m, thin, 0.6, 0.72), 0.6))
    col = m.mix(m.math('MULTIPLY', runs, m.math('MULTIPLY', m.math('ADD', 0.15, m.math('MULTIPLY', cluster, 0.85)), soot)), col, hexc(0x1a120e))
    col = m.mix(m.math('MULTIPLY', ramp(m, c.z, 2.2, 0.0), 0.5), col, hexc(0x231a14))
    wet = m.math('MAXIMUM', ramp(m, streak, 0.45, 0.7), m.math('MULTIPLY', ramp(m, mottle, 0.4, 0.8), 0.5))
    col = m.mix(m.math('MULTIPLY', wet, 0.32), col, cmul(m, col, 0.5))
    m.set('Base Color', col)
    # Wet brick darkens and takes a sheen, no more: a roughness around 0.35 on the faces, the joints staying matte.
    wet_k = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, mort), 0.27)
    if paint:
        wet_k = mix_float(m, gone, 0.2, wet_k)
    m.set('Roughness', m.math('SUBTRACT', rough, m.math('MULTIPLY', wet, wet_k)))
    bump(m, height, 0.55, 0.004)
    return m


def brownstone_surface(m, c, joints=True):
    """Brownstone: chocolate sandstone, its bedding along the courses, spalled faces paler and pitted, old cement
    patches, ashlar joints. Returns (color, roughness, height)."""
    mott = noise(m, c.P, 0.9, 5.0)
    bed = noise(m, vec(m, m.math('MULTIPLY', c.u, 0.7), m.math('MULTIPLY', c.v, 16.0), 0.0), 1.0, 4.0)
    col = m.mix(ramp(m, mott, 0.3, 0.75), hexc(0x4a3428), hexc(0x684b3a))
    col = m.mix(m.math('MULTIPLY', ramp(m, bed, 0.42, 0.75), 0.45), col, hexc(0x77584a))
    pores = noise(m, c.P, 90.0, 2.0)
    col = m.mix(m.math('MULTIPLY', ramp(m, pores, 0.55, 0.8), 0.3), col, hexc(0x2f221b))
    sp = noise(m, c.P, 1.1, 6.0, 0.65)
    spall = ramp(m, sp, 0.645, 0.66)
    col = m.mix(spall, col, m.mix(m.math('MULTIPLY', pores, 0.5), hexc(0x7e604c), hexc(0x5a4334)))
    rp = noise(m, vec(m, m.math('ADD', c.x, 13.0), m.math('ADD', c.y, 7.0), c.z), 0.7, 3.0)
    repair = ramp(m, rp, 0.69, 0.7)
    col = m.mix(repair, col, hexc(0x6b5346))
    height = m.math('SUBTRACT', m.math('MULTIPLY', pores, 0.35), m.math('MULTIPLY', spall, 1.0))
    rough = mix_float(m, spall, 0.76, 0.88)
    rough = mix_float(m, repair, rough, 0.6)
    if joints:
        course, block = 0.43, 1.15
        row = m.math('FLOOR', m.math('DIVIDE', c.v, course))
        ju = m.math('FRACT', m.math('DIVIDE', m.math('ADD', c.u, m.math('MULTIPLY', row, 0.57)), block))
        jv = m.math('FRACT', m.math('DIVIDE', c.v, course))
        du = m.math('MULTIPLY', m.math('MINIMUM', ju, m.math('SUBTRACT', 1.0, ju)), block)
        dv = m.math('MULTIPLY', m.math('MINIMUM', jv, m.math('SUBTRACT', 1.0, jv)), course)
        joint = m.math('MAXIMUM', smooth_less(m, du, 0.003, 0.0015), smooth_less(m, dv, 0.003, 0.0015))
        col = m.mix(m.math('MULTIPLY', joint, 0.7), col, hexc(0x2a1f19))
        height = m.math('SUBTRACT', height, m.math('MULTIPLY', joint, 0.8))
    return col, rough, height


def brownstone_material(name):
    m = core.Mat(name)
    c = Ctx(m)
    col, rough, height = brownstone_surface(m, c)
    col = weather(m, c, col, 0.55, 0.55, 0.35)
    m.set('Base Color', col)
    m.set('Roughness', m.math('SUBTRACT', rough, m.math('MULTIPLY', c.wet, 0.3)))
    bump(m, height, 0.45, 0.004)
    return m


# ------------------------------------------------------------------ the trim atlas

def s_stone(m, c, sp):
    h = sp['hex']
    pores = noise(m, c.P, 70.0, 2.0)
    col = m.mix(m.math('MULTIPLY', ramp(m, c.mottle, 0.35, 0.8), 0.6), hexc(h), hexc(_shade(h, 0.62)))
    col = m.mix(m.math('MULTIPLY', ramp(m, pores, 0.55, 0.8), 0.3), col, hexc(_shade(h, 0.75)))
    col = m.mix(m.math('MULTIPLY', c.edges, 0.3), col, hexc(_tint(h, 0.18)))
    col = weather(m, c, col, sp.get('soot', 0.6), 0.6, 0.35)
    rough = m.math('SUBTRACT', sp.get('rough', 0.72), m.math('MULTIPLY', c.wet, 0.3))
    return col, rough, 0.0, m.math('MULTIPLY', pores, 0.4)


def s_paint(m, c, sp):
    h = sp['hex']
    blot = ramp(m, noise(m, c.P, 7.0, 6.0, 0.6), 0.66, 0.72)
    ch = m.math('MAXIMUM', ramp(m, c.edges, 0.3, 0.55), m.math('MULTIPLY', blot, 0.8))
    ch = m.math('MULTIPLY', ch, sp.get('chips', 0.6))
    paint = m.mix(m.math('MULTIPLY', c.mottle, 0.3), hexc(h), hexc(_shade(h, 0.8)))
    col = m.mix(ch, paint, hexc(sp.get('under', 0x5f625f)))
    col = weather(m, c, col, sp.get('soot', 0.45), 0.5, 0.3)
    rough = mix_float(m, ch, m.math('SUBTRACT', sp.get('rough', 0.42), m.math('MULTIPLY', c.wet, 0.2)), 0.75)
    return col, rough, mix_float(m, ch, 0.0, sp.get('under_metal', 0.0)), m.math('MULTIPLY', ch, -0.5)


def s_iron(m, c, sp):
    rust = ramp(m, noise(m, c.P, 5.0, 5.0), 0.6, 0.74)
    rust = m.math('MAXIMUM', rust, m.math('MULTIPLY', ramp(m, c.edges, 0.3, 0.6), 0.7))
    col = m.mix(rust, hexc(sp.get('hex', 0x17181a)), m.mix(ramp(m, c.mottle, 0.3, 0.7), hexc(0x4a2414), hexc(0x6e3a1c)))
    col = weather(m, c, col, 0.15, 0.3, 0.3)
    return col, mix_float(m, rust, 0.42, 0.85), mix_float(m, rust, 0.4, 0.1), m.math('MULTIPLY', rust, 0.4)


def s_grate(m, c, sp):
    col, rough, metal, h = s_iron(m, c, sp)
    slot = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', c.y, 2 * math.pi / 0.032)))
    flat = ramp(m, m.math('ABSOLUTE', split(m, c.N)[2]), 0.6, 0.8)
    gap = m.math('MULTIPLY', ramp(m, slot, 0.75, 0.9), flat)
    return m.mix(gap, col, hexc(0x050505)), rough, metal, m.math('SUBTRACT', h, gap)


def s_galv(m, c, sp):
    spangle = noise(m, c.P, 60.0, 1.0, 0.5, 1.5)
    col = m.mix(m.math('MULTIPLY', spangle, 0.25), hexc(sp.get('hex', 0x9ea3a6)), hexc(0xc0c4c6))
    col = m.mix(m.math('MULTIPLY', c.mottle, 0.5), col, hexc(0x707477))
    col = weather(m, c, col, 0.5, 0.6, 0.3)
    return col, m.math('ADD', 0.45, m.math('MULTIPLY', spangle, 0.12)), 0.8, 0.0


def s_brass(m, c, sp):
    tarn = ramp(m, c.mottle, 0.35, 0.75)
    col = m.mix(m.math('MULTIPLY', tarn, 0.7), hexc(sp.get('hex', 0xb48c42)), hexc(0x4e4228))
    col = m.mix(m.math('MULTIPLY', c.edges, 0.6), col, hexc(0xd8b46a))
    return col, m.math('ADD', 0.28, m.math('MULTIPLY', tarn, 0.25)), 1.0, 0.0


def s_steel(m, c, sp):
    brush = noise(m, scaled(m, c.sep, 600.0, 600.0, 6.0), 4.0)
    col = m.mix(m.math('MULTIPLY', brush, 0.2), hexc(0x9da2a7), hexc(0xc3c7cb))
    col = m.mix(m.math('MULTIPLY', c.mottle, 0.3), col, hexc(0x5c6064))
    return col, m.math('ADD', 0.3, m.math('MULTIPLY', brush, 0.08)), 1.0, 0.0


def s_alu(m, c, sp):
    col = m.mix(m.math('MULTIPLY', c.mottle, 0.3), hexc(sp.get('hex', 0xbfc2c4)), hexc(0x8e9193))
    col = weather(m, c, col, 0.4, 0.4, 0.2)
    return col, 0.38, sp.get('metal', 0.8), 0.0


def s_plastic(m, c, sp):
    h = sp['hex']
    louver = ramp(m, m.math('SINE', m.math('MULTIPLY', c.v, 2 * math.pi / 0.022)), -0.2, 0.6)
    col = m.mix(m.math('MULTIPLY', ramp(m, c.mottle, 0.3, 0.8), 0.5), hexc(h), hexc(_shade(h, 0.78)))
    col = m.mix(m.math('MULTIPLY', ramp(m, c.streak, 0.45, 0.7), 0.6), col, hexc(0x5a4632))
    return col, m.math('ADD', 0.5, m.math('MULTIPLY', louver, 0.1)), 0.0, m.math('MULTIPLY', louver, 0.25)


def s_tar(m, c, sp):
    grit = noise(m, c.P, 120.0, 1.0)
    col = m.mix(m.math('MULTIPLY', grit, 0.4), hexc(0x1d1d1f), hexc(0x3a3936))
    return col, 0.88, 0.0, m.math('MULTIPLY', grit, 0.3)


def s_tile(m, c, sp):
    ts = sp.get('size', 0.024)
    fu = m.math('FRACT', m.math('DIVIDE', c.u, ts))
    fv = m.math('FRACT', m.math('DIVIDE', c.v, ts))
    gu = m.math('MINIMUM', fu, m.math('SUBTRACT', 1.0, fu))
    gv = m.math('MINIMUM', fv, m.math('SUBTRACT', 1.0, fv))
    grout = m.math('MAXIMUM', ramp(m, gu, 0.09, 0.03), ramp(m, gv, 0.09, 0.03))
    rnd, _ = white_noise(m, vec(m, m.math('FLOOR', m.math('DIVIDE', c.u, ts)), m.math('FLOOR', m.math('DIVIDE', c.v, ts)), 3.0))
    tile = m.mix(ramp(m, rnd, 0.55, 0.95), hexc(sp['hex']), hexc(sp.get('alt', _shade(sp['hex'], 0.8))))
    tile = m.mix(m.math('MULTIPLY', ramp(m, rnd, 0.97, 0.99), 0.8), tile, hexc(0xe6e2d6))
    col = m.mix(grout, tile, hexc(0x6e6a62))
    col = weather(m, c, col, 0.25, 0.75, 0.2)
    return col, mix_float(m, grout, 0.16, 0.85), 0.0, m.math('MULTIPLY', grout, -0.7)


def s_gate(m, c, sp, art=None):
    ridge = m.math('MULTIPLY', m.math('ADD', m.math('SINE', m.math('MULTIPLY', c.v, 2 * math.pi / 0.076)), 1.0), 0.5)
    col = m.mix(m.math('MULTIPLY', c.mottle, 0.6), hexc(0x8b9094), hexc(0x666b70))
    col = m.mix(m.math('MULTIPLY', ramp(m, ridge, 0.15, 0.0), 0.5), col, hexc(0x3e4246))
    rust = m.math('MULTIPLY', ramp(m, c.z, 0.7, 0.0), ramp(m, noise(m, c.P, 6.0, 4.0), 0.4, 0.7))
    col = m.mix(rust, col, hexc(0x5a3420))
    metal = mix_float(m, rust, 0.75, 0.1)
    rough = m.math('ADD', 0.42, m.math('MULTIPLY', c.mottle, 0.15))
    if art:
        gu = m.math('DIVIDE', m.math('SUBTRACT', c.u, art['a0']), art['a1'] - art['a0'])
        gv = m.math('DIVIDE', m.math('SUBTRACT', c.z, art['z0']), art['z1'] - art['z0'])
        ink, alpha, amet, arough = image_surface(m, art['paths'], vec(m, gu, gv, 0.0))
        inside = m.math('MULTIPLY', m.math('MULTIPLY', m.math('GREATER_THAN', gu, 0.0), m.math('LESS_THAN', gu, 1.0)),
                        m.math('MULTIPLY', m.math('GREATER_THAN', gv, 0.0), m.math('LESS_THAN', gv, 1.0)))
        f = m.math('MULTIPLY', m.math('MULTIPLY', alpha, inside), m.math('SUBTRACT', 1.0, m.math('MULTIPLY', ramp(m, ridge, 0.1, 0.0), 0.35)))
        col = m.mix(f, col, ink)
        metal = mix_float(m, f, metal, 0.0)
        rough = mix_float(m, f, rough, arough)
    col = weather(m, c, col, 0.35, 0.7, 0.3)
    return col, rough, metal, ridge


def s_granite(m, c, sp):
    f1 = noise(m, c.P, 160.0, 1.0)
    f2 = noise(m, vec(m, m.math('ADD', c.x, 3.1), c.y, c.z), 220.0, 1.0)
    col = m.mix(ramp(m, f1, 0.6, 0.66), hexc(sp.get('hex', 0x6a6864)), hexc(0x1c1b1a))
    col = m.mix(ramp(m, f2, 0.62, 0.68), col, hexc(0xb7b2aa))
    col = m.mix(m.math('MULTIPLY', c.edges, 0.3), col, hexc(0x9a968e))
    col = weather(m, c, col, 0.3, 0.6, 0.4)
    return col, m.math('SUBTRACT', 0.5, m.math('MULTIPLY', c.wet, 0.3)), 0.0, m.math('MULTIPLY', f1, 0.15)


def s_wood(m, c, sp):
    grain = noise(m, vec(m, m.math('MULTIPLY', c.u, 40.0), m.math('MULTIPLY', c.v, 1.6), m.math('MULTIPLY', c.x, 40.0)), 1.0, 6.0, 0.6, 1.5)
    h = sp.get('hex', 0x4a2c18)
    col = m.mix(grain, hexc(_shade(h, 0.7)), hexc(_tint(h, 0.15)))
    col = m.mix(m.math('MULTIPLY', c.edges, 0.5), col, hexc(_tint(h, 0.35)))
    col = weather(m, c, col, 0.2, 0.5, 0.2)
    return col, m.math('ADD', 0.3, m.math('MULTIPLY', c.edges, 0.3)), 0.0, m.math('MULTIPLY', grain, 0.2)


def s_black(m, c, sp):
    col = m.mix(m.math('MULTIPLY', c.mottle, 0.4), hexc(sp.get('hex', 0x151618)), hexc(0x2a2a28))
    return weather(m, c, col, 0.2, 0.3, 0.2), sp.get('rough', 0.5), sp.get('metal', 0.3), 0.0


def s_brown(m, c, sp):
    col, rough, height = brownstone_surface(m, c, joints=False)
    col = m.mix(m.math('MULTIPLY', c.edges, 0.35), col, hexc(0x7a5c48))
    col = weather(m, c, col, 0.55, 0.55, 0.35)
    return col, m.math('SUBTRACT', rough, m.math('MULTIPLY', c.wet, 0.3)), 0.0, height


def s_concrete(m, c, sp):
    blot = noise(m, c.P, 4.0, 5.0)
    pits = ramp(m, noise(m, c.P, 220.0, 1.0), 0.68, 0.72)
    h = sp.get('hex', 0x8f8b84)
    col = m.mix(m.math('MULTIPLY', blot, 0.5), hexc(h), hexc(_shade(h, 0.72)))
    col = m.mix(m.math('MULTIPLY', pits, 0.5), col, hexc(0x3a3936))
    col = weather(m, c, col, 0.4, 0.7, 0.4)
    return col, m.math('SUBTRACT', 0.85, m.math('MULTIPLY', c.wet, 0.4)), 0.0, m.math('SUBTRACT', blot, pits)


def s_plywood(m, c, sp):
    grain = noise(m, vec(m, m.math('MULTIPLY', c.u, 3.0), m.math('MULTIPLY', c.v, 30.0), 0.0), 1.0, 5.0, 0.6, 2.0)
    col = m.mix(grain, hexc(0x6e5c46), hexc(0x9a8466))
    col = m.mix(m.math('MULTIPLY', ramp(m, c.mottle, 0.3, 0.8), 0.6), col, hexc(0x5e5a54))
    col = weather(m, c, col, 0.6, 0.5, 0.4)
    return col, 0.85, 0.0, m.math('MULTIPLY', grain, 0.3)


def s_marble(m, c, sp):
    warp = noise(m, c.P, 1.8, 3.0, 0.5, 2.0)
    v = noise(m, vec(m, m.math('ADD', c.u, m.math('MULTIPLY', warp, 0.5)), m.math('ADD', c.v, m.math('MULTIPLY', warp, 0.7)), c.x), 3.2, 6.0)
    vein = ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', v, 0.5)), 0.012, 0.0)
    cloud = noise(m, c.P, 5.0, 3.0)
    col = m.mix(m.math('MULTIPLY', cloud, 0.3), hexc(sp.get('hex', 0xd2cdc2)), hexc(0xbab4a8))
    col = m.mix(m.math('MULTIPLY', vein, 0.35), col, hexc(0x8a857b))
    col = weather(m, c, col, 0.3, 0.6, 0.2)
    return col, 0.25, 0.0, 0.0


def s_leaf(m, c, sp):
    n = noise(m, c.P, 30.0, 3.0)
    return m.mix(n, hexc(0x1d3318), hexc(0x3e5a28)), 0.6, 0.0, m.math('MULTIPLY', n, 0.5)


SURF = {'stone': s_stone, 'paint': s_paint, 'iron': s_iron, 'grate': s_grate, 'galv': s_galv, 'brass': s_brass, 'steel': s_steel, 'alu': s_alu,
        'plastic': s_plastic, 'tar': s_tar, 'tile': s_tile, 'gate': s_gate, 'granite': s_granite, 'wood': s_wood, 'black': s_black,
        'brown': s_brown, 'concrete': s_concrete, 'plywood': s_plywood, 'marble': s_marble, 'leaf': s_leaf}


def trim_material(name, palette, gate=None):
    """Everything that isn't wall, glass or lit, in one material: each face's ss_kind picks its surface from palette
    ({kind: {'type': surface, 'hex': ..., ...}}), so the whole front's trim bakes into one texture set."""
    m = core.Mat(name)
    c = Ctx(m, edges=True)
    kind = attr(m, 'ss_kind')
    col, rough, metal, height = hexc(0x808080), 0.5, 0.0, 0.0
    for k, sp in palette.items():
        fn = SURF[sp['type']]
        kc, kr, km, kh = fn(m, c, sp, gate) if sp['type'] == 'gate' else fn(m, c, sp)
        mk = is_k(m, kind, TK[k])
        col = m.mix(mk, col, kc)
        rough = mix_float(m, mk, rough, kr)
        metal = mix_float(m, mk, metal, km)
        height = mix_float(m, mk, height, kh)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    bump(m, height, 0.45, 0.003)
    return m


# ------------------------------------------------------------------ glass and the lit interiors

def glass_material(name):
    """Dark windows: glass with a film of grime, and what's behind it at night: nothing, a roller shade pulled part
    way, sheers, vertical blinds, foil or newspaper taped up in an empty flat (ss_win: the window's u center, sill,
    seed; ss_win2: its width, height, look)."""
    m = core.Mat(name)
    c = Ctx(m)
    wc, wz0, seed = split(m, attr(m, 'ss_win', 'Vector'))
    ww, wh, look = split(m, attr(m, 'ss_win2', 'Vector'))
    uu = m.math('ADD', m.math('DIVIDE', m.math('SUBTRACT', c.u, wc), ww), 0.5)
    vv = m.math('DIVIDE', m.math('SUBTRACT', c.z, wz0), wh)
    smear = noise(m, c.P, 3.0, 3.0)
    col = m.mix(m.math('MULTIPLY', smear, 0.6), hexc(0x06080b), hexc(0x13171c))
    # A roller shade, pulled down to a level of its own, its hem bar a darker line.
    level = m.math('SUBTRACT', 0.82, m.math('MULTIPLY', seed, 0.62))
    below = m.math('SUBTRACT', vv, level)
    shade = ramp(m, below, -0.004, 0.004)
    hem = m.math('MULTIPLY', ramp(m, below, -0.004, 0.0), ramp(m, below, 0.02, 0.016))
    pick = m.math('FRACT', m.math('MULTIPLY', seed, 5.3))
    scol = m.mix(m.math('GREATER_THAN', pick, 0.35), hexc(0x403a30), hexc(0x2a313b))
    scol = m.mix(m.math('GREATER_THAN', pick, 0.7), scol, hexc(0x4b463d))
    scol = m.mix(hem, scol, hexc(0x15130f))
    col = m.mix(m.math('MULTIPLY', is_k(m, look, 1), shade), col, scol)
    # Sheers: soft folds.
    folds = m.math('MULTIPLY', m.math('ADD', m.math('SINE', m.math('ADD', m.math('MULTIPLY', m.math('MULTIPLY', uu, ww), 2 * math.pi / 0.075),
                                                                       m.math('MULTIPLY', noise(m, c.P, 4.0, 2.0), 3.0))), 1.0), 0.5)
    col = m.mix(is_k(m, look, 2), col, m.mix(folds, hexc(0x1f2226), hexc(0x34383d)))
    # Vertical blinds.
    vb = ramp(m, m.math('SINE', m.math('MULTIPLY', m.math('MULTIPLY', uu, ww), 2 * math.pi / 0.089)), -0.7, 0.7)
    col = m.mix(is_k(m, look, 3), col, m.mix(vb, hexc(0x202326), hexc(0x2f3236)))
    # Foil, crinkled.
    crinkle = noise(m, c.P, 26.0, 6.0, 0.7, 1.5)
    foil = is_k(m, look, 4)
    col = m.mix(foil, col, m.mix(crinkle, hexc(0x6e7074), hexc(0xb2b4b8)))
    # Newspaper: columns of print, the sheets' edges.
    rows = ramp(m, m.math('SINE', m.math('MULTIPLY', c.z, 2 * math.pi / 0.011)), 0.2, 0.9)
    cols_ = ramp(m, m.math('FRACT', m.math('DIVIDE', m.math('MULTIPLY', uu, ww), 0.055)), 0.92, 0.96)
    news = m.mix(m.math('MULTIPLY', m.math('MAXIMUM', m.math('MULTIPLY', rows, 0.35), cols_), ramp(m, noise(m, c.P, 40.0, 2.0), 0.3, 0.6)),
                 hexc(0x4e4b44), hexc(0x24221e))
    col = m.mix(is_k(m, look, 5), col, news)
    # Grime in the pane's lower corners.
    col = m.mix(m.math('MULTIPLY', ramp(m, vv, 0.12, 0.0), 0.4), col, hexc(0x14110d))
    m.set('Base Color', col)
    m.set('Roughness', mix_float(m, foil, m.math('ADD', 0.03, m.math('MULTIPLY', smear, 0.09)), 0.24))
    m.set('Metallic', m.math('MULTIPLY', foil, 0.9))
    m.set('Specular IOR Level', 0.5)
    bump(m, m.math('MULTIPLY', crinkle, foil), 0.7, 0.003)
    return m


def lit_material(name, strength=5.0, mailbox=None):
    """Everything seen lit through a window or a door: each surface lit by its room's lamp (ss_lamp: where it is;
    ss_room: the light's look, a seed, its power) falling off with distance and turning away from it, over its own
    albedo (ss_kind): the walls (a color per room), ceilings, floors, curtains and blinds glowing with the light behind
    them, furniture; the barber's checkered floor, tile, mirrors and chairs; the vestibule's marble, hex tile and
    brass mailboxes. Lamp shades, fixtures, screens and lanterns light themselves. Baked emissive."""
    m = core.Mat(name)
    c = Ctx(m)
    kind = attr(m, 'ss_kind')
    lamp = attr(m, 'ss_lamp', 'Vector')
    look, seed, power = split(m, attr(m, 'ss_room', 'Vector'))
    L = vmath(m, 'SUBTRACT', lamp, c.P)
    d2 = vmath(m, 'DOT_PRODUCT', L, L)
    ndl = vmath(m, 'DOT_PRODUCT', c.N, vmath(m, 'NORMALIZE', L))
    wrap = m.math('DIVIDE', m.math('ADD', ndl, 0.25), 1.25, clamp=True)
    fall = m.math('DIVIDE', power, m.math('ADD', 1.0, m.math('DIVIDE', d2, 0.8)))
    light = m.math('ADD', m.math('MULTIPLY', wrap, fall), m.math('MULTIPLY', power, 0.025))
    lampcol = hexc(LOOK_HEX[1])
    for k, h in LOOK_HEX.items():
        lampcol = m.mix(is_k(m, look, k), lampcol, hexc(h))

    def lit(albedo, gain=1.0):
        return cmul(m, ctimes(m, albedo, lampcol), m.math('MULTIPLY', light, gain))

    # Each room its own paint (a third of them papered).
    walls = [0xd6c8ab, 0xa7b597, 0x9aaec4, 0xd3a585, 0xdedad0, 0xc5ad86, 0xb98f8a]
    wall = hexc(walls[0])
    for i, h in enumerate(walls[1:]):
        wall = m.mix(m.math('GREATER_THAN', seed, (i + 1) / len(walls)), wall, hexc(h))
    stripes = ramp(m, m.math('SINE', m.math('MULTIPLY', c.u, 2 * math.pi / 0.11)), 0.2, 0.9)
    papered = m.math('GREATER_THAN', m.math('FRACT', m.math('MULTIPLY', seed, 7.0)), 0.66)
    wall = m.mix(m.math('MULTIPLY', stripes, m.math('MULTIPLY', papered, 0.18)), wall, cmul(m, wall, 0.7))
    picture_rnd, picture_col = white_noise(m, vec(m, m.math('FLOOR', m.math('MULTIPLY', c.u, 2.0)), m.math('FLOOR', m.math('MULTIPLY', c.v, 2.0)), seed))
    ceil = hexc(0xd2cec6)
    plank = m.math('FRACT', m.math('DIVIDE', c.v, 0.11))
    floor = m.mix(noise(m, scaled(m, c.sep, 2.0, 40.0, 2.0), 1.0, 4.0), hexc(0x4a3020), hexc(0x765034))
    floor = m.mix(ramp(m, m.math('MINIMUM', plank, m.math('SUBTRACT', 1.0, plank)), 0.05, 0.0), floor, hexc(0x24170e))
    fabrics = [0x8a2a22, 0xc29a44, 0x2e6464, 0xd6cab0, 0x6a4a78, 0x9b8a6a]
    fpick = m.math('FRACT', m.math('MULTIPLY', seed, 3.7))
    fabric = hexc(fabrics[0])
    for i, h in enumerate(fabrics[1:]):
        fabric = m.mix(m.math('GREATER_THAN', fpick, (i + 1) / len(fabrics)), fabric, hexc(h))
    pleat = m.math('ADD', 0.7, m.math('MULTIPLY', m.math('SINE', m.math('MULTIPLY', c.u, 2 * math.pi / 0.09)), 0.3))
    trans = m.math('MULTIPLY', m.math('ADD', 0.35, m.math('MULTIPLY', fall, 0.45)), pleat)
    curtain = cmul(m, ctimes(m, fabric, lampcol), trans)
    slats = ramp(m, m.math('SINE', m.math('MULTIPLY', c.z, 2 * math.pi / 0.026)), -0.3, 0.5)
    blinds = cmul(m, ctimes(m, hexc(0xe8dcc4), lampcol), m.math('MULTIPLY', m.math('ADD', 0.12, m.math('MULTIPLY', slats, 0.88)),
                                                                     m.math('ADD', 0.45, m.math('MULTIPLY', fall, 0.35))))
    shade = cmul(m, lampcol, m.math('ADD', 0.85, m.math('MULTIPLY', noise(m, c.P, 30.0, 2.0), 0.15)))
    fixture = m.mix(0.6, lampcol, (1.0, 1.0, 1.0, 1.0))
    # The barber's: 30 cm checkered floor, mint subway tile, mirrors catching the room, red vinyl, chrome.
    chk = m.math('FLOORED_MODULO', m.math('ADD', m.math('FLOOR', m.math('DIVIDE', c.x, 0.3)), m.math('FLOOR', m.math('DIVIDE', c.y, 0.3))), 2.0)
    checker = m.mix(chk, hexc(0x161616), hexc(0xd8d5cd))
    srow = m.math('FLOOR', m.math('DIVIDE', c.v, 0.076))
    su = m.math('FRACT', m.math('DIVIDE', m.math('ADD', c.u, m.math('MULTIPLY', srow, 0.075)), 0.15))
    sv = m.math('FRACT', m.math('DIVIDE', c.v, 0.076))
    sgrout = m.math('MAXIMUM', ramp(m, m.math('MINIMUM', su, m.math('SUBTRACT', 1.0, su)), 0.03, 0.01), ramp(m, m.math('MINIMUM', sv, m.math('SUBTRACT', 1.0, sv)), 0.06, 0.02))
    subway = m.mix(sgrout, hexc(0x9ccab4), hexc(0x6d7a72))
    # A mirror: the dim room in it, the lamp's streak.
    streak = m.math('MULTIPLY', ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', m.math('ADD', c.u, m.math('MULTIPLY', c.v, 0.4)), 1.45)), 0.5)), 0.06, 0.0), 0.7)
    mirror = m.mix(ramp(m, c.v, 1.0, 2.2), hexc(0x141a1f), hexc(0x56636d))
    mirror = m.mix(streak, mirror, hexc(0xc8d4dc))
    mirror = cmul(m, ctimes(m, mirror, lampcol), m.math('ADD', 0.25, m.math('MULTIPLY', fall, 0.35)))
    gloss = m.math('POWER', m.math('MAXIMUM', ndl, 0.0), 8.0)
    vinyl = m.mix(m.math('MULTIPLY', gloss, 0.5), hexc(0x7c1414), hexc(0xffd0c0))
    hexf = m.math('MAXIMUM', ramp(m, m.math('FRACT', m.math('DIVIDE', c.u, 0.025)), 0.92, 0.97), ramp(m, m.math('FRACT', m.math('DIVIDE', c.v, 0.025)), 0.92, 0.97))
    hexfloor = m.mix(m.math('MULTIPLY', hexf, 0.6), hexc(0xdcd8cf), hexc(0x77736b))
    border = m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', c.u, 0.9)), 0.5)), 0.06)
    hexfloor = m.mix(m.math('MULTIPLY', border, 0.8), hexfloor, hexc(0x1c1c1c))
    warp = noise(m, c.P, 2.0, 4.0, 0.5, 3.0)
    vn = noise(m, vec(m, m.math('ADD', c.u, m.math('MULTIPLY', warp, 0.6)), m.math('ADD', c.v, warp), c.x), 4.0, 8.0)
    marble = m.mix(m.math('MULTIPLY', ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', vn, 0.5)), 0.02, 0.0), 0.6), hexc(0xdcd7cc), hexc(0x6e6a62))
    # Brass mailboxes: a grid of little doors, each a white name slot and a lock.
    mb = hexc(0xb68c40)
    if mailbox:
        my0, mz0 = mailbox
        mu = m.math('DIVIDE', m.math('SUBTRACT', c.u, my0), 0.14)
        mv = m.math('DIVIDE', m.math('SUBTRACT', c.v, mz0), 0.17)
        fu, fv = m.math('FRACT', mu), m.math('FRACT', mv)
        gapm = m.math('MAXIMUM', ramp(m, m.math('MINIMUM', fu, m.math('SUBTRACT', 1.0, fu)), 0.04, 0.015), ramp(m, m.math('MINIMUM', fv, m.math('SUBTRACT', 1.0, fv)), 0.035, 0.012))
        slot = m.math('MULTIPLY', m.math('MULTIPLY', m.math('GREATER_THAN', fu, 0.15), m.math('LESS_THAN', fu, 0.85)),
                      m.math('MULTIPLY', m.math('GREATER_THAN', fv, 0.68), m.math('LESS_THAN', fv, 0.84)))
        lockd = m.math('SQRT', m.math('ADD', m.math('POWER', m.math('SUBTRACT', fu, 0.5), 2.0), m.math('POWER', m.math('MULTIPLY', m.math('SUBTRACT', fv, 0.35), 0.82), 2.0)))
        mb = m.mix(m.math('MULTIPLY', ramp(m, c.mottle, 0.4, 0.8), 0.4), mb, hexc(0x6e5428))
        mb = m.mix(gapm, mb, hexc(0x1a1408))
        mb = m.mix(slot, mb, hexc(0xe8e2d2))
        mb = m.mix(ramp(m, lockd, 0.07, 0.05), mb, hexc(0x2a2010))
    oak = m.mix(noise(m, scaled(m, c.sep, 40.0, 40.0, 1.5), 1.0, 5.0), hexc(0x3a2412), hexc(0x6a4422))
    # The pole: red, white, blue and white bands climbing round it.
    pc = vmath(m, 'SUBTRACT', c.P, lamp)
    px, py, _ = split(m, pc)
    ang = m.math('DIVIDE', m.math('ARCTAN2', py, px), 2 * math.pi)
    band = m.math('FRACT', m.math('ADD', m.math('DIVIDE', c.z, 0.16), ang))
    pole = m.mix(m.math('GREATER_THAN', band, 0.25), hexc(0xd21f2a), hexc(0xf4efe6))
    pole = m.mix(m.math('GREATER_THAN', band, 0.5), pole, hexc(0x1f45b0))
    pole = m.mix(m.math('GREATER_THAN', band, 0.75), pole, hexc(0xf4efe6))
    poster = m.mix(ramp(m, picture_rnd, 0.3, 0.7), picture_col, hexc(0xd8c8a0))
    cab = m.mix(ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', c.u, 0.45)), 0.5)), 0.49, 0.47), hexc(0xd9d4c8), hexc(0x6e6a62))
    em = lit(wall)
    for k, col in (('ceil', lit(ceil)), ('floor', lit(floor)), ('curtain', curtain), ('blinds', blinds), ('shade', shade), ('fixture', fixture),
                   ('furn', lit(hexc(0x2a1c14))), ('screen', (0.42, 0.58, 1.0, 1.0)), ('frame', lit(m.mix(0.5, picture_col, hexc(0x6a5a48)))),
                   ('plant', lit(hexc(0x26421f))), ('checker', lit(checker)), ('subway', lit(subway)), ('mirror', mirror), ('vinyl', lit(vinyl, 1.2)),
                   ('chrome', lit(hexc(0xc0c5ca), 1.4)), ('counter', lit(hexc(0xdcd6ca))), ('hex', lit(hexfloor)), ('marble', lit(marble, 1.1)),
                   ('mailbox', lit(mb, 1.5)), ('oak', lit(oak)), ('glow', hexc(0xffd9a8)), ('pole', cmul(m, pole, 0.8)), ('lantern', hexc(0xffe0b0)),
                   ('poster', lit(poster)), ('cabinet', lit(cab)), ('jar', (0.08, 0.45, 0.7, 1.0)), ('stair', lit(oak, 0.9))):
        em = m.mix(is_k(m, kind, IK[k]), em, col)
    m.set('Base Color', hexc(0x0e0d0c))
    m.set('Emission Color', em)
    m.set('Emission Strength', strength)
    m.set('Roughness', 0.35)
    return m


def print_material(m, signs):
    """The signs, numbers and labels: each sign's faces (ss_sign) show its own printed sheet, laid on by position
    along the face (left to right as you face it, so nothing reads mirrored in the game). Lit signs glow; a backlit
    one (a transom with gold leaf) glows only through its glass."""
    c = Ctx(m)
    sid = attr(m, 'ss_sign')
    col, rough, metal, emit = hexc(0x1c1c1c), 0.5, 0.0, hexc(0x000000)
    gmax = max([s['glow'] for s in signs] + [0.0])
    for s in signs:
        uu = m.math('DIVIDE', m.math('SUBTRACT', c.u, s['a0']), s['a1'] - s['a0'])
        vv = m.math('DIVIDE', m.math('SUBTRACT', c.z, s['z0']), s['z1'] - s['z0'])
        ink, alpha, met, rgh = image_surface(m, s['paths'], vec(m, uu, vv, 0.0))
        mk = is_k(m, sid, s['id'])
        col = m.mix(mk, col, ink)
        rough = mix_float(m, mk, rough, rgh)
        metal = mix_float(m, mk, metal, met)
        if s['glow'] > 0.0:
            e = cmul(m, ink, s['glow'] / gmax)
            if s['backlit']:
                e = cmul(m, e, m.math('SUBTRACT', 1.0, met))
            emit = m.mix(mk, emit, e)
    # A little of the street on them: grime and the wet.
    col = m.mix(m.math('MULTIPLY', ramp(m, c.mottle, 0.45, 0.85), 0.25), col, hexc(0x1e1a16))
    m.set('Base Color', col)
    m.set('Roughness', m.math('SUBTRACT', rough, m.math('MULTIPLY', c.wet, 0.15)))
    m.set('Metallic', metal)
    if gmax > 0.0:
        m.set('Emission Color', emit)
        m.set('Emission Strength', gmax)


# ------------------------------------------------------------------ geometry helpers

def _face(bm, vs, want):
    f = bm.faces.new(vs)
    f.normal_update()
    if f.normal.dot(Vector(want)) < 0.0:
        f.normal_flip()
    return f


def frame_matrix(center, n):
    """Local -> world for a face with outward normal n at center: local -X is out of the face, local +Y runs along it
    to the left as you face it, Z is up."""
    n = Vector(n).normalized()
    t = face_t(n)
    c = Vector(center)
    return Matrix(((-n.x, -t.x, 0.0, c.x), (-n.y, -t.y, 0.0, c.y), (-n.z, -t.z, 1.0, c.z), (0.0, 0.0, 0.0, 1.0)))


def wall_mesh(name, y0, y1, z0, z1, openings, depth, ends=None, top=True):
    """A masonry face at x = 0 from (y0, z0) to (y1, z1), facing -X, with rectangular openings (oy0, oy1, oz0, oz1)
    cut through it with reveals depth deep. Built on the openings' grid, so every face is a clean quad. ends: how far
    back the two end faces (returns) run (default: depth)."""
    ends = depth if ends is None else ends
    ys = sorted({round(min(max(v, y0), y1), 5) for v in [y0, y1] + [c for o in openings for c in (o[0], o[1])]})
    zs = sorted({round(min(max(v, z0), z1), 5) for v in [z0, z1] + [c for o in openings for c in (o[2], o[3])]})
    bm = bmesh.new()
    cache = {}

    def V(x, y, z):
        k = (round(x, 5), round(y, 5), round(z, 5))
        if k not in cache:
            cache[k] = bm.verts.new(k)
        return cache[k]

    def inside(cy, cz):
        return any(o[0] < cy < o[1] and o[2] < cz < o[3] for o in openings)

    openings_rev = [o for o in openings if len(o) < 5 or o[4]]

    for a, b in zip(ys, ys[1:]):
        for c, d in zip(zs, zs[1:]):
            if not inside((a + b) / 2, (c + d) / 2):
                _face(bm, [V(0, a, c), V(0, b, c), V(0, b, d), V(0, a, d)], (-1, 0, 0))
    for oy0, oy1, oz0, oz1, *_ in openings_rev:
        yy = [v for v in ys if oy0 - 1e-6 <= v <= oy1 + 1e-6]
        zz = [v for v in zs if oz0 - 1e-6 <= v <= oz1 + 1e-6]
        for a, b in zip(yy, yy[1:]):
            if oz0 > z0 + 1e-6:
                _face(bm, [V(0, a, oz0), V(0, b, oz0), V(depth, b, oz0), V(depth, a, oz0)], (0, 0, 1))
            if oz1 < z1 - 1e-6:
                _face(bm, [V(0, a, oz1), V(0, b, oz1), V(depth, b, oz1), V(depth, a, oz1)], (0, 0, -1))
        for c, d in zip(zz, zz[1:]):
            if oy0 > y0 + 1e-6:
                _face(bm, [V(0, oy0, c), V(0, oy0, d), V(depth, oy0, d), V(depth, oy0, c)], (0, 1, 0))
            if oy1 < y1 - 1e-6:
                _face(bm, [V(0, oy1, c), V(0, oy1, d), V(depth, oy1, d), V(depth, oy1, c)], (0, -1, 0))
    if ends > 0:
        for c, d in zip(zs, zs[1:]):
            if not inside(y0 + 1e-4, (c + d) / 2):
                _face(bm, [V(0, y0, c), V(0, y0, d), V(ends, y0, d), V(ends, y0, c)], (0, -1, 0))
            if not inside(y1 - 1e-4, (c + d) / 2):
                _face(bm, [V(0, y1, c), V(0, y1, d), V(ends, y1, d), V(ends, y1, c)], (0, 1, 0))
    if top:
        for a, b in zip(ys, ys[1:]):
            _face(bm, [V(0, a, z1), V(0, b, z1), V(depth, b, z1), V(depth, a, z1)], (0, 0, 1))
    return core.mesh_object(name, bm)


def prism_y(name, prof, y0, y1):
    """A closed (x, z) outline run along Y from y0 to y1 (sills, steps, hoods, brackets)."""
    bm = bmesh.new()
    a = [bm.verts.new((x, y0, z)) for x, z in prof]
    b = [bm.verts.new((x, y1, z)) for x, z in prof]
    n = len(prof)
    bm.faces.new(a)
    bm.faces.new(list(reversed(b)))
    for i in range(n):
        bm.faces.new((a[i], a[(i + 1) % n], b[(i + 1) % n], b[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return core.mesh_object(name, bm)


def molding(name, path, prof):
    """A molding: prof [(o, z)] (o out from the wall, z up; a closed outline) run along a horizontal polyline path
    [(x, y)] with its outward side on the left of travel, mitered at the corners and capped at the ends."""
    pts = [Vector((p[0], p[1], 0.0)) for p in path]
    ns = []
    for a, b in zip(pts, pts[1:]):
        d = (b - a).normalized()
        ns.append(Vector((-d.y, d.x, 0.0)))
    miters = []
    for i in range(len(pts)):
        if i == 0:
            mv = ns[0]
        elif i == len(pts) - 1:
            mv = ns[-1]
        else:
            s = (ns[i - 1] + ns[i]).normalized()
            mv = s / max(s.dot(ns[i]), 0.2)
        miters.append(mv)
    bm = bmesh.new()
    rings = [[bm.verts.new(pts[i] + miters[i] * o + Vector((0.0, 0.0, z))) for o, z in prof] for i in range(len(pts))]
    n = len(prof)
    for r0, r1 in zip(rings, rings[1:]):
        for k in range(n):
            bm.faces.new((r0[k], r0[(k + 1) % n], r1[(k + 1) % n], r1[k]))
    bm.faces.new(rings[0])
    bm.faces.new(list(reversed(rings[-1])))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return core.mesh_object(name, bm)


def console_profile(depth, height, scroll=True):
    """A scrolled bracket's side outline in (x, z), its back on the wall (x = 0), top at z = 0, reaching out to -depth."""
    pts = [(0.0, 0.0), (-depth, 0.0), (-depth, -0.1 * height)]
    for i in range(1, 11):
        t = i / 10
        x = -depth * (0.18 + 0.82 * (1.0 - t) ** 1.7)
        z = -0.1 * height - t * 0.72 * height
        pts.append((x, z))
    if scroll:
        r = 0.09 * height
        cx, cz = -depth * 0.18 - r * 0.2, -0.82 * height - r
        for i in range(1, 8):
            a = math.pi / 2 + i * math.pi / 8
            pts.append((cx + r * math.cos(a) * 1.1, cz + r * math.sin(a)))
    pts.append((0.0, -height))
    return pts


def pleat_panel(name, y0, y1, x, z0, z1, amp=0.03, pitch=0.085):
    """A curtain's pleated panel hanging at x from y0 to y1, facing -X."""
    n = max(2, int(abs(y1 - y0) / (pitch / 2)))
    bm = bmesh.new()
    lo, hi = [], []
    for i in range(n + 1):
        y = y0 + (y1 - y0) * i / n
        dx = amp * (1 if i % 2 else -1) * 0.5
        lo.append(bm.verts.new((x + dx, y, z0)))
        hi.append(bm.verts.new((x + dx, y, z1)))
    for i in range(n):
        _face(bm, [lo[i], lo[i + 1], hi[i + 1], hi[i]], (-1, 0, 0))
    return core.mesh_object(name, bm)


def quad_obj(name, pts, want):
    bm = bmesh.new()
    _face(bm, [bm.verts.new(p) for p in pts], want)
    return core.mesh_object(name, bm)


def xform(objs, M):
    """Moves parts by M, their lamp positions with them."""
    for o in objs:
        o.data.transform(M)
        a = o.data.attributes.get('ss_lamp')
        if a is not None:
            vs = [0.0] * (len(a.data) * 3)
            a.data.foreach_get('vector', vs)
            out = []
            for i in range(0, len(vs), 3):
                p = M @ Vector(vs[i:i + 3])
                out += [p.x, p.y, p.z]
            a.data.foreach_set('vector', out)
    return objs


# ------------------------------------------------------------------ a front under construction

class Front:
    def __init__(self, key, width, wall_t=0.36, seed=1):
        self.key = key
        self.name = MESH[key]
        self.p = PREFIX[key]
        self.W = width
        self.wall_t = wall_t
        self.rnd = random.Random(seed)
        self.objs = []
        self.signs = []
        self.planar = []          # (test, axes, rect): wall faces mapped flat
        self.wall_boxes = []      # (test, rect): wall faces packed into a rect of their own
        self.wall_box = (0.0, 0.905, 1.0, 1.0)   # the rest of the wall's faces
        self.trim_planar = []     # (test, axes, rect): big flat trim faces given a sharp region of the atlas
        self.trim_box = (0.0, 0.0, 1.0, 1.0)
        self.collapse = []        # tests: wall faces nobody looks at closely (a back, an underside), all on one texel
        # Where a camera can be: the sidewalks and the street in front (and the windows across it). A face that turns
        # its back on all of them is never seen, and is cut before the UVs are laid out.
        self.cams = [(x, y, z) for x in (-0.45, -1.6, -4.0, -9.0, -18.0) for y in (-1.5 * width, -0.5 * width, 0.0, 0.5 * width, 1.5 * width)
                     for z in (0.25, 1.7, 6.0, 14.0)]

    def setup(self, wall, palette, gate=None, mailbox=None, lit_strength=5.0):
        self.wall = wall
        self.palette = palette
        self.trim = trim_material(f'{self.p}_trim', palette, gate)
        self.glass = glass_material(f'{self.p}_glass')
        self.lit = lit_material(f'{self.p}_lit', lit_strength, mailbox)
        self.print = core.Mat(f'{self.p}_print')

    def add(self, *objs):
        for o in objs:
            if isinstance(o, (list, tuple)):
                self.objs.extend(o)
            else:
                self.objs.append(o)

    # Parts: trim (by kind), lit (by lit kind, with its room's light), wall.
    def tag(self, o, kind):
        assert kind in self.palette, f'{self.key}: no {kind} in the palette'
        core.assign(o, self.trim)
        set_attr(o, 'ss_kind', TK[kind])
        return o

    def blk(self, x0, x1, y0, y1, z0, z1, kind, r=0.0, seg=1):
        o = parts.rbox(kind, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), r, seg)
        return self.tag(o, kind)

    def rod(self, a, b, r, kind, n=8):
        return self.tag(parts.rod(kind, a, b, r, n), kind)

    def prism(self, prof, y0, y1, kind):
        return self.tag(prism_y(kind, prof, y0, y1), kind)

    def molding(self, path, prof, kind):
        return self.tag(molding(kind, path, prof), kind)

    def lit_tag(self, o, ikind, lamp, room):
        core.assign(o, self.lit)
        set_attr(o, 'ss_kind', IK[ikind])
        set_attr(o, 'ss_lamp', lamp)
        set_attr(o, 'ss_room', room)
        return o

    def lblk(self, x0, x1, y0, y1, z0, z1, ikind, lamp, room, r=0.0):
        o = parts.rbox(ikind, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), r, 1)
        return self.lit_tag(o, ikind, lamp, room)

    def lquad(self, pts, want, ikind, lamp, room):
        return self.lit_tag(quad_obj(ikind, pts, want), ikind, lamp, room)

    def wall_part(self, o):
        core.assign(o, self.wall)
        return o

    def pane(self, x, y0, y1, z0, z1, win, look, seed, M=None):
        """A dark pane (a quad facing -X at x); win = (y center, sill, width, height) of its window, in local coords."""
        o = quad_obj('glass', [(x, y0, z0), (x, y1, z0), (x, y1, z1), (x, y0, z1)], (-1, 0, 0))
        core.assign(o, self.glass)
        wy, wz0, ww, wh = win
        c = Vector((x, wy, wz0))
        n = Vector((-1.0, 0.0, 0.0))
        if M is not None:
            c = M @ c
            n = M.to_3x3() @ n
        set_attr(o, 'ss_win', (c.dot(face_t(n)), c.z, seed))
        set_attr(o, 'ss_win2', (ww, wh, GLASS[look]))
        return o

    def sign(self, label, w, h, draw, center, normal=(-1.0, 0.0, 0.0), depth=0.015, glow=0.0, backlit=False, px=1024, body=True):
        """A printed sign w x h (m) whose face is centered at center facing normal; draw(sheet, W, H) lays it out in
        millimeters. Returns its board (a thin box behind the face, depth deep)."""
        s = Sheet(w * 1000.0, h * 1000.0)
        draw(s, w * 1000.0, h * 1000.0)
        paths = s.render(f'fac_{self.p}_{label}', px)
        sid = len(self.signs) + 1
        n = Vector(normal).normalized()
        t = face_t(n)
        cv = Vector(center)
        ac = cv.dot(t)
        self.signs.append(dict(id=sid, paths=paths, a0=ac - w / 2, a1=ac + w / 2, z0=cv.z - h / 2, z1=cv.z + h / 2, glow=glow, backlit=backlit))
        o = parts.rbox('sign', (depth / 2, 0.0, 0.0), (depth, w, h), 0.0)
        o.data.transform(frame_matrix(cv, n))
        core.assign(o, self.print)
        set_attr(o, 'ss_sign', sid)
        return o

    def finish(self):
        if self.signs:
            print_material(self.print, self.signs)
        else:
            self.print.set('Base Color', hexc(0x202020))
        # Fixed slot order: wall, trim, glass, lit, print. Every part gets all five in that order (its faces on its
        # own), so joining keeps it.
        mats = [self.wall.m, self.trim.m, self.glass.m, self.lit.m, self.print.m]
        order = {m.name: i for i, m in enumerate(mats)}
        for o in self.objs:
            idx = order[o.data.materials[0].name]
            o.data.materials.clear()
            for mt in mats:
                o.data.materials.append(mt)
            o.data.polygons.foreach_set('material_index', [idx] * len(o.data.polygons))
        obj = core.join(self.objs, self.name)
        obj.name = obj.data.name = self.name
        self.cull(obj)
        core.finish_hard_surface(obj, 30.0)
        wall = core.material_is(obj, self.wall.m.name)
        trim = core.material_is(obj, self.trim.m.name)
        groups = [(lambda f, t=t: wall(f) and t(f), 'planar', axes, rect) for t, axes, rect in self.planar]
        groups += [(lambda f, t=t: wall(f) and t(f), 'box', None, rect) for t, rect in self.wall_boxes]
        groups.append((wall, 'box', None, self.wall_box))
        groups += [(lambda f, t=t: trim(f) and t(f), 'planar', axes, rect) for t, axes, rect in self.trim_planar]
        groups.append((trim, 'box', None, self.trim_box))
        for s in obj.material_slots:
            if s.material.name not in (self.wall.m.name, self.trim.m.name):
                groups.append((core.material_is(obj, s.material.name), 'box', None, (0.0, 0.0, 1.0, 1.0)))
        groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
        core.uv_layout(obj, groups)
        if self.collapse:
            r = self.wall_boxes[0][1] if self.wall_boxes else self.wall_box
            spot = ((r[0] + r[2]) / 2, (r[1] + r[3]) / 2)
            bm = bmesh.new()
            bm.from_mesh(obj.data)
            uv = bm.loops.layers.uv.active
            for f in bm.faces:
                if f.material_index == 0 and any(t(f) for t in self.collapse):
                    for lp in f.loops:
                        lp[uv].uv = spot
            bm.to_mesh(obj.data)
            bm.free()
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        print(f'[facades] {self.name}: {tris} triangles; slots ' + ', '.join(f'{i} {s.material.name}' for i, s in enumerate(obj.material_slots)))
        return obj

    def cull(self, obj):
        """Deletes the faces no camera can see: turned away from every place one can stand (backs against the wall,
        undersides on the ground, the insides of boxes pressed into the masonry)."""
        me = obj.data
        n = len(me.polygons)
        nrm = np.empty(n * 3, dtype=np.float64)
        cen = np.empty(n * 3, dtype=np.float64)
        me.polygons.foreach_get('normal', nrm)
        me.polygons.foreach_get('center', cen)
        nrm, cen = nrm.reshape(n, 3), cen.reshape(n, 3)
        cams = np.array(self.cams, dtype=np.float64)
        facing = np.einsum('fk,fk->f', nrm, cen)
        seen = (nrm @ cams.T - facing[:, None] > 1e-4).any(axis=1)
        mi = np.empty(n, dtype=np.int32)
        me.polygons.foreach_get('material_index', mi)
        kill = np.nonzero(~seen & (mi != 0))[0]
        bm = bmesh.new()
        bm.from_mesh(me)
        bm.faces.ensure_lookup_table()
        bmesh.ops.delete(bm, geom=[bm.faces[i] for i in kill], context='FACES')
        bm.to_mesh(me)
        bm.free()
        print(f'[facades] {self.name}: culled {len(kill)} of {n} faces no camera sees')

    def back_plate(self, z0, z1, x=ROOM_BACK):
        """Closes the front's back (seen only in reviews: massing hides it in the game) with one plain wall quad."""
        hw = self.W / 2
        self.add(self.wall_part(quad_obj('back', [(x, -hw, z0), (x, hw, z0), (x, hw, z1), (x, -hw, z1)], (1, 0, 0))))
        self.collapse.append(lambda f, x=x: f.normal.x > 0.9 and f.calc_center_median().x > x - 0.01)

    def layout_wall(self, flats, px=(2048, 2048), region=(0.0, 0.0, 0.86, 1.0), hw=None, rest=None):
        """The wall's texture: its flat faces [(test, axes, w, h)] side by side at one texel size (square texels) over
        most of it; the reveals and tops in the larger leftover strip, the returns (party walls, mostly hidden) in the
        other."""
        u0, v0, u1, v1 = region
        pw, ph = px
        sc = min((u1 - u0 - 0.004 * (len(flats) - 1)) * pw / sum(f[2] for f in flats), (v1 - v0) * ph / max(f[3] for f in flats))
        u = u0
        for test, axes, w, h in flats:
            rect = (u, v0, u + w * sc / pw, v0 + h * sc / ph)
            self.planar.append((test, axes, rect))
            u = rect[2] + 0.004
        top = v0 + max(f[3] for f in flats) * sc / ph
        right_r = (u + 0.002, 0.0, 1.0, 1.0)
        top_r = (0.0, top + 0.005, u, 1.0)
        area_r, area_t = (1.0 - u), u * (1.0 - top)
        big, small = (right_r, top_r) if area_r >= area_t else (top_r, right_r)
        if min(small[2] - small[0], small[3] - small[1]) < 0.03:
            # One strip only: split it between the reveals and the returns.
            if big[2] - big[0] > big[3] - big[1]:
                mid = big[0] + (big[2] - big[0]) * 0.6
                big, small = (big[0], big[1], mid, big[3]), (mid + 0.004, big[1], big[2], big[3])
            else:
                mid = big[1] + (big[3] - big[1]) * 0.6
                big, small = (big[0], big[1], big[2], mid), (big[0], mid + 0.004, big[2], big[3])
        if hw is not None:
            rest = (lambda f, hw=hw: abs(f.normal.y) > 0.9 and abs(f.calc_center_median().y) > hw - 0.01)
        if rest is not None:
            self.wall_boxes.append((rest, small))
        self.wall_box = big


def fit_rect(w_m, h_m, region, px):
    """The largest rectangle in region (u0, v0, u1, v1) holding w_m x h_m meters with square texels (texture px)."""
    u0, v0, u1, v1 = region
    pw, ph = px
    s = min((u1 - u0) * pw / w_m, (v1 - v0) * ph / h_m)
    return (u0, v0, u0 + w_m * s / pw, v0 + h_m * s / ph)


# ------------------------------------------------------------------ windows, rooms, ironwork

def window(F, y, z0, w, h, look, seed, floor_z, ceil_z, room_w=None, panes=(1, 1), raise_=0.0, lintel='flat', sill='stone',
           ac=False, guard=False, M=None, curtains=None, blinds=None, depth=None, sash='sash', stone='stone', box_ext=0.13):
    """One double-hung window in a reveal at y (center), sill z0, w x h: frame, two sashes (the lower one raised by
    raise_), glass or the lit room behind, the masonry sill and lintel, maybe an air conditioner or a guard. Built in
    the face's local frame and moved by M."""
    d = depth if depth is not None else F.wall_t
    sb = 0.1
    y0, y1, z1 = y - w / 2, y + w / 2, z0 + h
    out = []
    fw = 0.06
    out += [F.blk(sb, sb + 0.13, y0, y0 + fw, z0, z1, sash, 0.004), F.blk(sb, sb + 0.13, y1 - fw, y1, z0, z1, sash, 0.004),
            F.blk(sb, sb + 0.13, y0 + fw, y1 - fw, z1 - 0.07, z1, sash, 0.004)]
    out.append(F.prism([(sb - 0.035, z0), (sb + 0.13, z0), (sb + 0.13, z0 + 0.045), (sb - 0.035, z0 + 0.018)], y0 + fw, y1 - fw, sash))
    ya, yb, za, zb = y0 + fw, y1 - fw, z0 + 0.045, z1 - 0.07
    zm = (za + zb) / 2 + 0.03
    if ac:
        raise_ = 0.4
    raise_ = min(raise_, zb - zm - 0.06)
    lit = look in ROOM_LOOKS or look in ('drawn', 'blinds')
    room_look = look if look in ROOM_LOOKS else None
    win = (y, z0, w, h)
    gl = 'dark' if lit else look
    cols, rows = panes

    def sash_frame(xa, xb, sz0, sz1, top, bot):
        s = [F.blk(xa, xb, ya, ya + 0.045, sz0, sz1, sash, 0.003), F.blk(xa, xb, yb - 0.045, yb, sz0, sz1, sash, 0.003),
             F.blk(xa, xb, ya + 0.045, yb - 0.045, sz1 - top, sz1, sash, 0.003), F.blk(xa, xb, ya + 0.045, yb - 0.045, sz0, sz0 + bot, sash, 0.003)]
        py0, py1, pz0, pz1 = ya + 0.045, yb - 0.045, sz0 + bot, sz1 - top
        for i in range(1, cols):
            yy = py0 + (py1 - py0) * i / cols
            s.append(F.blk(xa + 0.008, xb - 0.008, yy - 0.011, yy + 0.011, pz0, pz1, sash))
        for j in range(1, rows):
            zz = pz0 + (pz1 - pz0) * j / rows
            s.append(F.blk(xa + 0.008, xb - 0.008, py0, py1, zz - 0.011, zz + 0.011, sash))
        if not lit:
            s.append(F.pane((xa + xb) / 2, py0, py1, pz0, pz1, win, gl, seed, M))
        return s

    out += sash_frame(sb + 0.02, sb + 0.06, zm - 0.02, zb, 0.05, 0.035)
    out += sash_frame(sb + 0.07, sb + 0.11, za + raise_, zm + 0.02 + raise_, 0.035, 0.075)
    if raise_ > 0.0 and not lit:
        # Under the raised sash: the room's dark (an AC fills it, or a screen).
        out.append(F.pane(sb + 0.12, ya, yb, za, za + raise_, win, 'dark', seed, M))
    # Masonry sill and lintel.
    if sill == 'stone' or sill == 'brown':
        k = stone if sill == 'stone' else 'brown'
        out.append(F.prism([(-0.075, z0 - 0.09), (sb, z0 - 0.09), (sb, z0), (-0.075, z0 - 0.03), (-0.085, z0 - 0.035), (-0.085, z0 - 0.08)],
                           y0 - 0.1, y1 + 0.1, k))
    # The stone heads and jambs reach a few millimeters into the opening, so their soffits and returns are the
    # reveal's first centimeters instead of lying in the brick's plane (which z-fights).
    zl = z1 - 0.003
    if lintel in ('flat', 'key'):
        out.append(F.blk(-0.025, 0.05, y0 - 0.13, y1 + 0.13, zl, z1 + 0.24, stone, 0.006))
        if lintel == 'key':
            out.append(F.prism([(-0.055, z1 - 0.03), (0.05, z1 - 0.03), (0.05, z1 + 0.29), (-0.055, z1 + 0.29)], y - 0.08, y + 0.08, stone))
            out.append(F.prism([(-0.05, z1 + 0.29), (0.05, z1 + 0.29), (0.05, z1 + 0.31), (-0.05, z1 + 0.31)], y - 0.1, y + 0.1, stone))
    elif lintel == 'hood':
        out.append(F.blk(-0.03, 0.05, y0 - 0.12, y1 + 0.12, zl, z1 + 0.2, stone, 0.006))
        hood = [(0.0, 0.0), (0.06, 0.0), (0.06, 0.02), (0.09, 0.05), (0.09, 0.1), (0.13, 0.12), (0.16, 0.17), (0.17, 0.2), (0.17, 0.24), (0.0, 0.24)]
        out.append(F.molding([(-0.03, y0 - 0.22), (-0.03, y1 + 0.22)], hood, stone))
        out[-1].data.transform(Matrix.Translation((0.0, 0.0, z1 + 0.2)))
        for yy in (y0 - 0.16, y1 + 0.16):
            out.append(F.prism([(x - 0.03, z + z1 + 0.2) for x, z in console_profile(0.12, 0.34)], yy - 0.05, yy + 0.05, stone))
    elif lintel == 'eared':
        k = 0.12
        out += [F.blk(-0.035, 0.03, y0 - k, y0 + 0.003, z0, zl, stone, 0.006), F.blk(-0.035, 0.03, y1 - 0.003, y1 + k, z0, zl, stone, 0.006),
                F.blk(-0.035, 0.03, y0 - k - 0.06, y1 + k + 0.06, zl, z1 + 0.16, stone, 0.006),
                F.blk(-0.035, 0.03, y0 - k - 0.06, y0 - k, z1 - 0.12, z1, stone, 0.006), F.blk(-0.035, 0.03, y1 + k, y1 + k + 0.06, z1 - 0.12, z1, stone, 0.006)]
        cap = [(0.0, 0.0), (0.05, 0.0), (0.05, 0.03), (0.09, 0.06), (0.11, 0.1), (0.11, 0.13), (0.0, 0.13)]
        out.append(F.molding([(-0.035, y0 - k - 0.12), (-0.035, y1 + k + 0.12)], cap, stone))
        out[-1].data.transform(Matrix.Translation((0.0, 0.0, z1 + 0.16)))
    # The room behind a lit window, or what's drawn across it.
    if room_look:
        out += room(F, y, w, d, floor_z, ceil_z, room_w or (w + 1.1), room_look, seed, z0, z1, curtains, blinds, sb)
    elif look == 'drawn':
        lamp, rm = (sb + 1.0, y, z0 + 0.7), (1, seed, 1.1)
        gap = 0.02 + 0.06 * seed
        out.append(F.lit_tag(pleat_panel('curtain', ya - 0.02, y - gap / 2, sb + 0.2, za, zb + 0.06), 'curtain', lamp, rm))
        out.append(F.lit_tag(pleat_panel('curtain', y + gap / 2, yb + 0.02, sb + 0.2, za, zb + 0.06), 'curtain', lamp, rm))
        out.append(F.lquad([(sb + 0.3, ya, za), (sb + 0.3, yb, za), (sb + 0.3, yb, zb), (sb + 0.3, ya, zb)], (-1, 0, 0), 'glow', lamp, rm))
    elif look == 'blinds':
        lamp, rm = (sb + 1.0, y, z0 + 1.0), (1, seed, 1.1)
        out.append(F.lquad([(sb + 0.15, ya, za), (sb + 0.15, yb, za), (sb + 0.15, yb, zb), (sb + 0.15, ya, zb)], (-1, 0, 0), 'blinds', lamp, rm))
    if ac:
        aw = min(0.66, w - 0.2)
        out.append(F.blk(-0.34, sb + 0.32, y - aw / 2, y + aw / 2, za + 0.01, za + 0.39, 'ac', 0.012, 2))
        out.append(F.blk(-0.345, -0.33, y - aw / 2 + 0.04, y + aw / 2 - 0.04, za + 0.05, za + 0.34, 'ac'))
        for s in (-1, 1):
            out.append(F.blk(sb + 0.07, sb + 0.1, (ya if s < 0 else y + aw / 2), (y - aw / 2 if s < 0 else yb), za, za + 0.38, 'ac'))
            out.append(F.rod((-0.3, y + s * (aw / 2 - 0.05), z0 - 0.08), (-0.02, y + s * (aw / 2 - 0.05), z0 - 0.42), 0.012, 'iron', 4))
    if guard:
        gx = -0.065
        n = max(3, int(w / 0.11))
        for i in range(n + 1):
            yy = y0 + 0.03 + (w - 0.06) * i / n
            out.append(F.blk(gx - 0.007, gx + 0.007, yy - 0.007, yy + 0.007, z0 + 0.02, z1 - 0.02, 'iron'))
        for zz in (z0 + 0.06, (z0 + z1) / 2, z1 - 0.06):
            out.append(F.blk(gx - 0.012, gx + 0.012, y0 + 0.01, y1 - 0.01, zz - 0.02, zz + 0.02, 'iron'))
        for zz in (z0 + 0.06, z1 - 0.06):
            for yy in (y0 + 0.03, y1 - 0.03):
                out.append(F.blk(gx, 0.0, yy - 0.015, yy + 0.015, zz - 0.02, zz + 0.02, 'iron'))
    if M is not None:
        xform(out, M)
    return out


def room(F, y, w, x0, fz, cz, rw, look, seed, z0, z1, curtains, blinds, sb):
    """A lit room behind a window: an open box (back wall, sides, floor, ceiling) facing the window, its lamp, and the
    few things seen of it from the street: a shade or a pendant, a kitchen's ceiling light and cabinets, a television's
    glow, pictures, a bookcase, curtains and blinds."""
    rnd = random.Random(int(seed * 1e6) + 7)
    x1 = x0 + RD
    ry0, ry1 = y - rw / 2, y + rw / 2
    side = 1 if rnd.random() < 0.5 else -1
    lk = LOOKS[look]
    out = []
    pend = False
    if look == 'warm':
        pend = rnd.random() < 0.45
        lamp = (x0 + RD * 0.45, y + rnd.uniform(-0.3, 0.3), cz - 0.68) if pend else (x0 + RD * 0.62, y + side * (rw / 2 - 0.45), fz + 1.12)
        power = 1.3
    elif look == 'cool':
        lamp, power = (x0 + RD * 0.5, y, cz - 0.3), 1.45
    elif look == 'tv':
        lamp, power = (x0 + RD * 0.62, y + side * (rw / 2 - 0.3), fz + 0.8), 1.1
    elif look == 'laptop':
        lamp, power = (x0 + 0.55, y + 0.1, fz + 0.86), 0.75
    elif look == 'stair':
        lamp, power = (x0 + 1.0, y, cz - 0.35), 1.0
    else:
        lamp, power = (x0 + RD * 0.6, y - side * (rw / 2 - 0.5), fz + 1.25), 1.2
    rm = (lk, seed, power)
    q = F.lquad
    out += [q([(x1, ry0, fz), (x1, ry1, fz), (x1, ry1, cz), (x1, ry0, cz)], (-1, 0, 0), 'wall', lamp, rm),
            q([(x0, ry0, fz), (x1, ry0, fz), (x1, ry0, cz), (x0, ry0, cz)], (0, 1, 0), 'wall', lamp, rm),
            q([(x0, ry1, fz), (x1, ry1, fz), (x1, ry1, cz), (x0, ry1, cz)], (0, -1, 0), 'wall', lamp, rm),
            q([(x0, ry0, fz), (x1, ry0, fz), (x1, ry1, fz), (x0, ry1, fz)], (0, 0, 1), 'floor', lamp, rm),
            q([(x0, ry0, cz), (x1, ry0, cz), (x1, ry1, cz), (x0, ry1, cz)], (0, 0, -1), 'ceil', lamp, rm)]
    lx, ly, lz = lamp
    if look == 'warm':
        if pend:
            out.append(F.lit_tag(parts.rod('cord', (lx, ly, cz), (lx, ly, lz + 0.12), 0.006, 4), 'furn', lamp, rm))
            out.append(F.lit_tag(parts.lathe_at('shade', [(0.03, 0.1), (0.2, -0.08), (0.2, -0.1), (0.0, -0.1)], Matrix.Translation(lamp), 16), 'shade', lamp, rm))
        else:
            out.append(F.lit_tag(parts.lathe_at('shade', [(0.0, -0.13), (0.19, -0.13), (0.13, 0.11), (0.0, 0.11)], Matrix.Translation(lamp), 16), 'shade', lamp, rm))
            out.append(F.lblk(lx - 0.25, lx + 0.25, ly - 0.25, ly + 0.25, fz, fz + 0.62, 'furn', lamp, rm))
            out.append(F.lit_tag(parts.rod('stem', (lx, ly, fz + 0.62), (lx, ly, lz - 0.13), 0.012, 6), 'furn', lamp, rm))
    elif look == 'cool':
        out.append(F.lblk(lx - 0.35, lx + 0.35, ly - 0.15, ly + 0.15, cz - 0.05, cz, 'fixture', lamp, rm))
        out.append(F.lblk(x1 - 0.34, x1, ry0 + 0.15, ry1 - 0.15, fz + 1.45, fz + 2.2, 'cabinet', lamp, rm))
        out.append(F.lblk(x1 - 0.6, x1, ry0 + 0.15, ry1 - 0.15, fz + 0.86, fz + 0.9, 'counter', lamp, rm))
    elif look == 'tv':
        wy = ry0 + 0.06 if side < 0 else ry1 - 0.06
        out.append(F.lblk(lx - 0.5, lx + 0.5, wy - 0.04, wy + 0.04, fz + 0.62, fz + 1.18, 'screen', lamp, rm))
        out.append(F.lblk(lx - 0.6, lx + 0.6, wy - 0.22 * side - 0.2, wy - 0.22 * side + 0.2, fz, fz + 0.5, 'furn', lamp, rm))
    elif look == 'stair':
        # A flight climbing across the window, its rail on balusters, the hall's dome light.
        n = 9
        for i in range(n):
            sy = y - 1.1 + i * 0.26
            out.append(F.lblk(x0 + 0.55, x0 + 1.45, sy, sy + 0.27, fz - 0.6, fz - 0.6 + 0.19 * (i + 1), 'stair', lamp, rm))
            out.append(F.lit_tag(parts.rod('baluster', (x0 + 0.58, sy + 0.13, fz - 0.6 + 0.19 * (i + 1)), (x0 + 0.58, sy + 0.13, fz + 0.3 + 0.19 * (i + 1)), 0.01, 4), 'oak', lamp, rm))
        out.append(F.lit_tag(parts.rod('rail', (x0 + 0.58, y - 1.1, fz + 0.4), (x0 + 0.58, y - 1.1 + n * 0.26, fz + 0.4 + 0.19 * n), 0.025, 8), 'oak', lamp, rm))
        out.append(F.lit_tag(parts.lathe_at('dome', [(0.0, -0.1), (0.13, -0.06), (0.15, 0.0), (0.0, 0.0)], Matrix.Translation((lx, ly, cz)), 16), 'fixture', lamp, rm))
    elif look == 'laptop':
        out.append(F.lblk(x0 + 0.08, x0 + 0.72, y - 0.7, y + 0.7, fz + 0.72, fz + 0.76, 'furn', lamp, rm))
        out.append(F.lblk(x0 + 0.62, x0 + 0.7, y - 0.66, y + 0.66, fz, fz + 0.72, 'furn', lamp, rm))
        out.append(F.lit_tag(parts.rbox('lid', (0, 0, 0), (0.008, 0.34, 0.23), 0.0), 'furn', lamp, rm))
        out[-1].data.transform(Matrix.Translation((x0 + 0.42, y + 0.1, fz + 0.88)) @ Matrix.Rotation(math.radians(-14), 4, 'Y'))
        out.append(F.lblk(x1 - 0.03, x1, y - 0.45, y + 0.25, fz + 1.35, fz + 2.0, 'poster', lamp, rm))
    else:
        out.append(F.lit_tag(parts.lathe_at('shade', [(0.0, -0.12), (0.17, -0.12), (0.12, 0.1), (0.0, 0.1)], Matrix.Translation(lamp), 16), 'shade', lamp, rm))
        out.append(F.lit_tag(parts.rod('stem', (lx, ly, fz), (lx, ly, lz - 0.12), 0.012, 6), 'furn', lamp, rm))
        for i in range(3):
            out.append(F.lblk(x1 - 0.03, x1, ry0 + 0.3 + i * 0.55, ry0 + 0.7 + i * 0.55, fz + 1.3, fz + 1.9, 'poster', lamp, rm))
    # Pictures on the back wall, a bookcase against it in some.
    if look in ('warm', 'tv', 'red') and rnd.random() < 0.7:
        for i in range(rnd.choice((1, 2))):
            py = y + rnd.uniform(-rw / 2 + 0.4, rw / 2 - 0.4)
            pz = fz + rnd.uniform(1.45, 1.75)
            pw, ph = rnd.uniform(0.3, 0.6), rnd.uniform(0.3, 0.5)
            out.append(F.lblk(x1 - 0.03, x1, py - pw / 2, py + pw / 2, pz, pz + ph, 'frame', lamp, rm))
    if look in ('warm', 'tv') and rnd.random() < 0.5:
        by = ry0 + 0.05 if rnd.random() < 0.5 else ry1 - 0.95
        out.append(F.lblk(x1 - 0.32, x1, by, by + 0.9, fz, fz + 1.95, 'furn', lamp, rm))
    if curtains is None:
        curtains = look in ('warm', 'red') and rnd.random() < 0.6
    if curtains:
        cut = 0.15 + 0.2 * rnd.random()
        cx = x0 + 0.04
        out.append(F.lit_tag(pleat_panel('curtain', y - w / 2 - 0.22, y - w / 2 + w * cut, cx, z0 - 0.3, z1 + 0.22), 'curtain', lamp, rm))
        out.append(F.lit_tag(pleat_panel('curtain', y + w / 2 - w * cut, y + w / 2 + 0.22, cx, z0 - 0.3, z1 + 0.22), 'curtain', lamp, rm))
        out.append(F.lit_tag(parts.rod('rod', (cx - 0.02, y - w / 2 - 0.28, z1 + 0.26), (cx - 0.02, y + w / 2 + 0.28, z1 + 0.26), 0.01, 6), 'furn', lamp, rm))
    if blinds:
        bz = z1 - 0.07 - (z1 - z0) * blinds
        out.append(F.lquad([(sb + 0.15, y - w / 2 + 0.06, bz), (sb + 0.15, y + w / 2 - 0.06, bz), (sb + 0.15, y + w / 2 - 0.06, z1 - 0.07),
                            (sb + 0.15, y - w / 2 + 0.06, z1 - 0.07)], (-1, 0, 0), 'blinds', lamp, rm))
        out.append(F.lblk(sb + 0.13, sb + 0.17, y - w / 2 + 0.07, y + w / 2 - 0.07, bz - 0.02, bz, 'furn', lamp, rm))
    return out


def fire_escape(F, ya, yb, zs, deep=1.15, ladder=True, M=None):
    """An iron fire escape across ya..yb: a grated platform under the windows at each z in zs, railings of square
    bars, stairs between the platforms, brackets back to the wall, the drop ladder hooked up under the lowest."""
    out = []
    for k, z in enumerate(zs):
        # The grating sits down inside its angle-iron frame (its top a few millimeters under the frame's).
        out.append(F.blk(-deep + 0.05, -0.03, ya + 0.05, yb - 0.05, z - 0.03, z - 0.006, 'grate'))
        out += [F.blk(-deep, -deep + 0.05, ya, yb, z - 0.11, z, 'iron'), F.blk(-deep, -0.03, ya, ya + 0.05, z - 0.11, z, 'iron'),
                F.blk(-deep, -0.03, yb - 0.05, yb, z - 0.11, z, 'iron')]
        for y in (ya + 0.08, (ya + yb) / 2, yb - 0.08):
            out.append(F.rod((-deep + 0.08, y, z - 0.08), (-0.02, y, z - 0.62), 0.016, 'iron', 4))
        rz = z + 0.95
        for zz, hh in ((rz, 0.04), (z + 0.48, 0.03)):
            out += [F.blk(-deep, -deep + 0.03, ya, yb, zz - hh / 2, zz + hh / 2, 'iron'), F.blk(-deep, -0.03, ya, ya + 0.03, zz - hh / 2, zz + hh / 2, 'iron'),
                    F.blk(-deep, -0.03, yb - 0.03, yb, zz - hh / 2, zz + hh / 2, 'iron')]
        n = int((yb - ya) / 0.115)
        for j in range(n + 1):
            y = ya + 0.015 + (yb - ya - 0.03) * j / n
            out.append(F.blk(-deep + 0.008, -deep + 0.022, y - 0.007, y + 0.007, z, rz, 'iron'))
        for j in range(1, 9):
            x = -deep + (deep - 0.05) * j / 9
            for y in (ya + 0.015, yb - 0.015):
                out.append(F.blk(x - 0.007, x + 0.007, y - 0.007, y + 0.007, z, rz, 'iron'))
        # The stairs up to the next platform, along the front, alternating ends; the hatch they rise through.
        if k + 1 < len(zs):
            z2 = zs[k + 1]
            s0, s1 = (ya + 0.3, yb - 1.25) if k % 2 == 0 else (yb - 0.3, ya + 1.25)
            xs = -deep + 0.36
            for sd in (-0.27, 0.27):
                out.append(F.rod((xs + sd, s0, z), (xs + sd, s1, z2), 0.022, 'iron', 4))
            steps = 11
            for j in range(1, steps):
                t = j / steps
                yy, zz = s0 + (s1 - s0) * t, z + (z2 - z) * t
                out.append(F.blk(xs - 0.27, xs + 0.27, yy - 0.085, yy + 0.085, zz - 0.02, zz, 'grate'))
            out.append(F.rod((xs - 0.3, s0, z + 0.88), (xs - 0.3, s1, z2 + 0.88), 0.014, 'iron', 6))
            out.append(F.rod((xs - 0.3, s0, z), (xs - 0.3, s0, z + 0.88), 0.012, 'iron', 4))
    if ladder:
        z1 = zs[0]
        ly = yb - 0.62 if len(zs) > 1 else (ya + yb) / 2
        for sd in (-0.21, 0.21):
            out.append(F.blk(-deep + 0.1, -deep + 0.16, ly + sd - 0.012, ly + sd + 0.012, z1 - 2.5, z1 + 0.9, 'iron'))
        for j in range(12):
            zz = z1 - 2.35 + j * 0.28
            out.append(F.rod((-deep + 0.13, ly - 0.21, zz), (-deep + 0.13, ly + 0.21, zz), 0.01, 'iron', 6))
        out.append(F.rod((-deep + 0.13, ly, z1 + 0.9), (-deep + 0.13, ly, z1 + 1.1), 0.01, 'iron', 4))
    if M is not None:
        xform(out, M)
    return out


def pot_plant(F, x, y, z, r=0.13):
    """A terracotta pot with something leggy in it, on a fire escape or a sill."""
    out = [F.tag(parts.lathe_at('pot', [(0.0, 0.0), (r * 0.75, 0.0), (r, r * 1.5), (r * 1.08, r * 1.6), (r * 1.08, r * 1.75), (0.0, r * 1.75)],
                                Matrix.Translation((x, y, z)), 16), 'pot')]
    for i in range(5):
        a = i * 2.4
        tip = (x + math.cos(a) * r * 1.6, y + math.sin(a) * r * 1.6, z + r * (3.2 + (i % 3) * 0.8))
        b = parts.rbox('leaf', tip, (r * 1.1, r * 1.1, r * 0.9), r * 0.3, 2)
        out.append(F.tag(b, 'leaf'))
    return out


def downspout(F, y, z_top, x_off=-0.09, knots=(), M=None):
    """A galvanized leader from a gutter head under the cornice down to the sidewalk, strapped every couple of meters;
    knots [(z, x)] carry it out round projections on the way down (top to bottom)."""
    out = [F.blk(x_off - 0.13, x_off + 0.06, y - 0.14, y + 0.14, z_top - 0.3, z_top, 'galv', 0.01)]
    pts = [(x_off, y, z_top - 0.3)] + [(x, y, z) for z, x in knots]
    pts.append((pts[-1][0], y, 0.12))
    for a, b in zip(pts, pts[1:]):
        out.append(F.rod(a, b, 0.045, 'galv', 12))
    out.append(F.rod((pts[-1][0], y, 0.12), (pts[-1][0] - 0.16, y, 0.02), 0.045, 'galv', 12))
    for a, b in zip(pts, pts[1:]):
        if abs(a[0] - b[0]) > 0.01:
            continue
        za, zb = sorted((a[2], b[2]))
        zz = za + 0.6
        while zz < zb - 0.3:
            out.append(F.blk(a[0] - 0.05, 0.0, y - 0.055, y + 0.055, zz, zz + 0.03, 'galv'))
            zz += 2.1
    if M is not None:
        xform(out, M)
    return out


def cable(F, pts, r=0.006):
    return F.tag(parts.tube('cable', pts, r, 6, 6), 'black')


def cornice(F, path, z, height, project, modillions=True, dentils=True, kind='cornice', consoles=True, console_h=None):
    """A pressed-metal (or wood) cornice along path [(x, y)] (outward on the left of travel) with its underside at z:
    frieze fascia, bed molding, dentils, the soffit on modillion brackets, the corona and a cyma crown, mitered at the
    corners; big scrolled consoles at its ends."""
    H, P = height, project
    k = P / 0.7
    prof = [(0.0, 0.0), (0.05, 0.0), (0.05, 0.08 * H), (0.08, 0.1 * H), (0.08, 0.36 * H), (0.12 * k, 0.38 * H), (0.14 * k, 0.43 * H),
            (0.2 * k, 0.46 * H), (0.22 * k, 0.5 * H), (0.24 * k, 0.56 * H), (0.8 * P, 0.58 * H), (0.84 * P, 0.62 * H),
            (0.84 * P, 0.74 * H), (0.88 * P, 0.76 * H), (0.93 * P, 0.81 * H), (0.98 * P, 0.88 * H), (P, 0.92 * H), (P, H), (0.0, H)]
    o = F.molding(path, prof, kind)
    o.data.transform(Matrix.Translation((0.0, 0.0, z)))
    out = [o]
    segs = list(zip(path, path[1:]))
    for i, (a, b) in enumerate(segs):
        A, B = Vector((a[0], a[1], 0.0)), Vector((b[0], b[1], 0.0))
        L = (B - A).length
        d = (B - A).normalized()
        M = frame_matrix(A, (-d.y, d.x, 0.0))
        lo = 0.06 if i == 0 else 0.2 * k + 0.08
        hi = L - (0.06 if i == len(segs) - 1 else 0.2 * k + 0.08)
        local = []
        if dentils:
            yy = lo
            while yy < hi:
                local.append(F.blk(-0.2 * k, -0.05, yy - 0.025, yy + 0.025, z + 0.38 * H, z + 0.46 * H, kind))
                yy += 0.1
        if modillions:
            lo_m = 0.35 if i == 0 else 0.8 * P + 0.1
            hi_m = L - (0.3 if i == len(segs) - 1 else 0.8 * P + 0.1)
            span = hi_m - lo_m
            n = int(span / 0.62)
            spots = [lo_m + span * j / n for j in range(n + 1)] if n >= 1 else ([(lo_m + hi_m) / 2] if span > -0.2 and L > 0.5 else [])
            for yy in spots:
                prof_m = [(x - 0.24 * k, zz + z + 0.58 * H) for x, zz in console_profile(0.8 * P - 0.24 * k - 0.02, 0.12 * H, scroll=False)]
                local.append(F.prism(prof_m, yy - 0.05, yy + 0.05, kind))
        if consoles:
            ch = console_h or 0.95 * H
            ends = ([0.11] if i == 0 else []) + ([L - 0.11] if i == len(segs) - 1 else [])
            for yy in ends:
                prof_c = [(x - 0.06, zz + z + 0.58 * H) for x, zz in console_profile(0.8 * P - 0.06, ch)]
                local.append(F.prism(prof_c, yy - 0.09, yy + 0.09, kind))
        out += xform(local, M)
    return out


def lantern(F, x, y, z, s=1.0, wall_n=(-1, 0, 0)):
    """A carriage lamp on a scrolled arm: a tapered black frame, frosted glass lit from inside, a cap and finial; its
    back plate against the wall at x."""
    out = []
    lamp = (x - 0.28 * s, y, z)
    rm = (7, 0.5, 1.0)
    out.append(F.blk(x - 0.03, x, y - 0.06 * s, y + 0.06 * s, z - 0.2 * s, z + 0.12 * s, 'black', 0.005))
    out.append(F.rod((x - 0.02, y, z - 0.05 * s), (x - 0.24 * s, y, z - 0.05 * s), 0.012 * s, 'black', 6))
    out.append(F.rod((x - 0.02, y, z - 0.16 * s), (x - 0.2 * s, y, z - 0.07 * s), 0.009 * s, 'black', 6))
    cx = x - 0.28 * s
    gl = parts.lathe_at('glass', [(0.0, -0.13 * s), (0.065 * s, -0.13 * s), (0.085 * s, 0.1 * s), (0.0, 0.1 * s)], Matrix.Translation((cx, y, z)), 4)
    gl.data.transform(Matrix.Translation((cx, y, z)) @ Matrix.Rotation(math.radians(45), 4, 'Z') @ Matrix.Translation((-cx, -y, -z)))
    out.append(F.lit_tag(gl, 'lantern', lamp, rm))
    for a in range(4):
        ang = math.radians(45 + a * 90)
        r0, r1 = 0.068 * s * 1.41, 0.088 * s * 1.41
        out.append(F.rod((cx + r0 * math.cos(ang) * 0.7, y + r0 * math.sin(ang) * 0.7, z - 0.13 * s),
                         (cx + r1 * math.cos(ang) * 0.7, y + r1 * math.sin(ang) * 0.7, z + 0.1 * s), 0.006 * s, 'black', 4))
    out.append(F.tag(parts.lathe_at('cap', [(0.0, 0.1 * s), (0.11 * s, 0.1 * s), (0.11 * s, 0.115 * s), (0.03 * s, 0.19 * s), (0.012 * s, 0.24 * s), (0.0, 0.25 * s)],
                                    Matrix.Translation((cx, y, z)), 4), 'black'))
    out[-1].data.transform(Matrix.Translation((cx, y, z)) @ Matrix.Rotation(math.radians(45), 4, 'Z') @ Matrix.Translation((-cx, -y, -z)))
    out.append(F.tag(parts.lathe_at('bottom', [(0.0, -0.17 * s), (0.02 * s, -0.17 * s), (0.075 * s, -0.13 * s), (0.0, -0.12 * s)], Matrix.Translation((cx, y, z)), 4), 'black'))
    return out


# ------------------------------------------------------------------ signs' artwork

def outlined(s, text, x, y, size, color, outline, face='Black', align='CENTER', tracking=1.0, width=None, rough=0.4, metal=0.0, squeeze=1.0, shadow=None):
    """Lettering with a keyline round it (drawn eight times offset, then the face), optionally squeezed narrower."""
    w = width if width is not None else size * 0.06
    objs = []
    if shadow:
        sc, dx, dy = shadow
        objs.append(s.text(text, x + dx, y + dy, size, sc, face=face, align=align, tracking=tracking, rough=0.6))
    for i in range(8):
        a = i * math.pi / 4
        objs.append(s.text(text, x + w * math.cos(a), y + w * math.sin(a), size, outline, face=face, align=align, tracking=tracking, rough=0.5))
    objs.append(s.text(text, x, y, size, color, face=face, align=align, tracking=tracking, rough=rough, metal=metal))
    if squeeze != 1.0:
        for o in objs:
            o.scale.x = squeeze
            o.location.x = x + (o.location.x - x) * squeeze
    return objs


def squeezed(s, text, x, y, size, color, squeeze=0.85, **kw):
    o = s.text(text, x, y, size, color, **kw)
    o.scale.x = squeeze
    return o


def worn(s, W, H, n=60, color=0x000000, alpha=0.25, seed=3):
    """Scuffs, chips and scratches over a print, so it isn't factory-new: ragged little flakes (more along the bottom
    and the edges, where weather and hands get at it) and fine scratches. Sized from the sheet's shorter side."""
    r = random.Random(seed)
    lo = min(W, H)
    for _ in range(n):
        if r.random() < 0.55:
            x = r.uniform(0, W)
            y = H * r.random() ** 2.2 if r.random() < 0.6 else r.uniform(0, H)
            if r.random() < 0.3:
                x = r.choice((r.uniform(0, 0.06), r.uniform(0.94, 1.0))) * W
            rad = r.uniform(0.006, 0.03) * lo
            k = r.randint(5, 8)
            pts = [(x + rad * r.uniform(0.45, 1.0) * math.cos(2 * math.pi * (i + r.uniform(-0.3, 0.3)) / k),
                    y + rad * r.uniform(0.45, 1.0) * math.sin(2 * math.pi * (i + r.uniform(-0.3, 0.3)) / k)) for i in range(k)]
            s.poly(pts, color, alpha=alpha, rough=0.7)
        else:
            x, y = r.uniform(0, W), r.uniform(0, H)
            s.rect(x, y, r.uniform(0.04, 0.2) * lo, r.uniform(0.002, 0.005) * lo, color, rot=r.uniform(-0.5, 0.5), alpha=alpha * 0.7, rough=0.6)


# ------------------------------------------------------------------ SM_Facade_Walkup

def walkup():
    """A 1905 walk-up: five storeys of common-bond red brick over a cast-iron shopfront."""
    F = Front('walkup', 13.5, wall_t=0.36, seed=1905)
    W, hw = F.W, F.W / 2
    SHOP, BAND = 3.35, 4.38          # the shop opening's head; the storefront cornice's top
    FH, NF = 3.05, 4
    def fz(k):
        return BAND + (k - 1) * FH
    TOP = fz(NF + 1)                 # the cornice's underside
    CH = 0.95                        # the cornice's height
    WALL_TOP = TOP + CH
    BAYS = [-5.2, -2.6, 0.0, 2.6, 5.2]
    WW, WH, SILL = 1.1, 1.72, 0.85
    GATE = (-6.3, -1.2)
    BARB = (1.2, 6.3)

    def gate_art(s, Wm, Hm):
        r = random.Random(77)
        # Wheat-pasted posters (the Embercrest's series, two side by side) and tags over the bottom of everything.
        for i, px in enumerate((Wm * 0.56, Wm * 0.56 + 640)):
            s.rect(px, 1150, 600, 900, 0x0a1020, rough=0.7)
            brand.lockup(s, px + 300, 1720, 470, sub='POKER SERIES')
            s.text('NORTHSIDE QUALIFIERS', px + 300, 1395, 34, brand.GOLD, face='Bold', align='CENTER', tracking=1.4, rough=0.7)
            s.text('SAT 7PM · THE EMBERCREST', px + 300, 1330, 30, brand.ASH, face='Bold', align='CENTER', tracking=1.2, rough=0.7)
            s.rect(px, 1150, 600, 40, brand.EMBER, rough=0.7)
            for _ in range(7):
                tx, ty = px + r.uniform(0, 600), 1150 + r.choice((0, 900)) + r.uniform(-30, 30)
                s.poly([(tx, ty), (tx + r.uniform(30, 90), ty + r.uniform(-20, 20)), (tx + r.uniform(0, 60), ty + r.uniform(-60, 60))], 0x7f8488, rough=0.5)
        s.rect(Wm * 0.08, 1500, 520, 380, 0xe9e4d6, rough=0.8)
        s.text('LOST CAT', Wm * 0.08 + 260, 1790, 60, 0x111111, face='Black', align='CENTER', rough=0.8)
        s.text('"BISCUIT" · ORANGE · 1 EYE', Wm * 0.08 + 260, 1700, 26, 0x222222, face='Bold', align='CENTER', rough=0.8)
        s.text('555-0141', Wm * 0.08 + 260, 1560, 50, 0x111111, face='Black', align='CENTER', rough=0.8)
        tags = [('ZEPH', 0xe8e8e8, 0x1b1b1b, 600, 180, -0.06, 420), ('REK 5', 0x3fa9f5, 0x0b1f40, 1750, 520, 0.08, 330), ('NSD', 0xf2c230, 0x3a1a00, 2650, 140, -0.03, 520),
                ('DUKE', 0xff4d8d, 0x2a0010, 3750, 380, 0.05, 380), ('MOSE', 0xe8e8e8, 0x000000, 4500, 820, -0.1, 230), ('Z', 0xf2c230, 0x111111, 1150, 1250, 0.2, 260)]
        for text, col, keyl, x, y, rot, size in tags:
            for o in outlined(s, text, x, y, size, col, keyl, face='Black', width=size * 0.07, rough=0.55):
                o.rotation_euler.z = rot
                o.scale.y = 1.15
            for _ in range(4):
                dx = x + r.uniform(-size, size)
                s.rect(dx, y - r.uniform(60, 220), 7, r.uniform(60, 200), col, rough=0.55)
        s.text('SHOP OPEN 7AM', Wm * 0.5, 2620, 130, 0x111111, face='Black', align='CENTER', tracking=1.1, rough=0.6)

    gsheet = Sheet((GATE[1] - GATE[0]) * 1000.0, 2970.0)
    gate_art(gsheet, (GATE[1] - GATE[0]) * 1000.0, 2970.0)
    gpaths = gsheet.render('fac_wu_gate', 2048)
    palette = dict(stone=dict(type='stone', hex=0xb0a48a), sash=dict(type='paint', hex=0xd4ccb6, under=0x6a5a48, chips=0.7),
                   front=dict(type='paint', hex=0x1c3a2a, under=0x4a3a2c, chips=0.75, under_metal=0.3),
                   cornice=dict(type='paint', hex=0xcac1aa, under=0x5d6266, chips=0.55, soot=0.3, under_metal=0.6),
                   door=dict(type='paint', hex=0x4e1c16, under=0x6b4a30), iron=dict(type='iron'), grate=dict(type='grate'), galv=dict(type='galv'),
                   brass=dict(type='brass'), ac=dict(type='plastic', hex=0xc6bfae), tar=dict(type='tar'), tile=dict(type='tile', hex=0x2c5a46, alt=0x214637, size=0.022),
                   gate=dict(type='gate'), granite=dict(type='granite'), black=dict(type='black'), steel=dict(type='steel'), marble=dict(type='marble'),
                   pot=dict(type='paint', hex=0x9a5234, under=0x6a3a24, chips=0.3), leaf=dict(type='leaf'))
    F.setup(brick_material('wu_wall', 0x7b3a2a, 0x5a2a1f, 0x77715f, light=0x9a6450, dark=0x3a1d17, soot=0.6, bond='common'), palette,
            gate=dict(paths=gpaths, a0=-GATE[1], a1=-GATE[0], z0=0.06, z1=3.03))

    # Which window is which, floor by floor (bays left to right in Y), and their air conditioners.
    plan = {1: ['shade', 'dark', 'warm', 'sheer', 'tv'], 2: ['cool', 'shade', 'dark', 'blinds', 'dark'],
            3: ['dark', 'drawn', 'shade', 'warm', 'vblind'], 4: ['warm', 'foil', 'paper', 'shade', 'red']}
    acs = {(1, 3), (2, 4), (3, 2)}
    raised = {(1, 2): 0.28, (4, 0): 0.2}
    openings = []
    for k in range(1, NF + 1):
        for i, y in enumerate(BAYS):
            z0 = fz(k) + SILL
            openings.append((y - WW / 2, y + WW / 2, z0, z0 + WH))
            F.add(window(F, y, z0, WW, WH, plan[k][i], F.rnd.random(), fz(k), fz(k + 1) - 0.3, room_w=2.45, panes=(2, 1),
                         raise_=raised.get((k, i), 0.0), lintel='key', ac=(k, i) in acs, blinds=0.35 if (k, i) == (3, 3) else None))
    wall = F.wall_part(wall_mesh('brick', -hw, hw, SHOP, WALL_TOP, openings, F.wall_t, ends=ROOM_BACK))
    F.add(wall)
    F.layout_wall([(lambda f: f.normal.x < -0.9, ('-y', 'z', -hw, hw, SHOP, WALL_TOP), W, WALL_TOP - SHOP)], hw=hw)
    F.add(F.blk(0.05, ROOM_BACK, -hw + 0.01, hw - 0.01, WALL_TOP - 0.06, WALL_TOP - 0.02, 'tar'))
    F.back_plate(0.0, WALL_TOP)

    # The cornice, and the brick frieze's stone band under it.
    F.add(cornice(F, [(0.0, -hw), (0.0, hw)], TOP, CH, 0.72))
    F.add(F.blk(-0.04, 0.0, -hw, hw, TOP - 0.42, TOP - 0.3, 'stone', 0.006))

    # The shopfront: cast-iron pilasters, the sign band and its cornice.
    piers = [(-hw, GATE[0]), (GATE[1], -0.62), (0.62, BARB[0]), (BARB[1], hw)]
    for a, b in piers:
        F.add(F.blk(-0.17, F.wall_t, a, b, 0.0, 0.3, 'granite', 0.01))
        F.add(F.blk(-0.14, F.wall_t, a, b, 0.3, SHOP, 'front', 0.006))
        F.add(F.blk(-0.165, -0.14, a + 0.07, b - 0.07, 0.62, 2.85, 'front', 0.006))
        nr = 3 if b - a > 0.5 else 2
        for j in range(nr):
            yy = a + 0.07 + (b - a - 0.14) * (j + 0.5) / nr
            F.add(F.rod((-0.165, yy, 0.86), (-0.165, yy, 2.6), 0.014, 'front', 8))
        for zz in (0.62, 2.85):
            if zz < 1 and (a, b) == (GATE[1], -0.62):
                continue       # the standpipe's plate goes there
            F.add(F.tag(parts.disc('rosette', (-0.17, (a + b) / 2, zz + (0.12 if zz < 1 else -0.12)), (1, 0, 0), 0.045, 0.02, 12), 'front'))
        F.add(F.blk(-0.18, -0.14, a - 0.02, b + 0.02, 0.3, 0.42, 'front', 0.006))
        cap = [(0.0, 0.0), (0.04, 0.0), (0.04, 0.05), (0.07, 0.08), (0.08, 0.14), (0.0, 0.14)]
        o = F.molding([(-0.14, a - 0.02), (-0.14, b + 0.02)], cap, 'front')
        o.data.transform(Matrix.Translation((0.0, 0.0, SHOP - 0.2)))
        F.add(o)
    F.add(F.blk(-0.07, 0.0, -hw, hw, SHOP, BAND - 0.36, 'front', 0.004))
    # The shop storey's party-wall returns (the brick's own start at the sign band): without them the end of the
    # row would show into the barber's and the cleaners'.
    for s in (-1, 1):
        ry, rx = s * (hw + 0.0005), F.wall_t - 0.05     # tucked under the pier's bevel, a hair proud of its end
        F.add(F.wall_part(quad_obj('shop_ret', [(rx, ry, 0.0), (ROOM_BACK, ry, 0.0), (ROOM_BACK, ry, SHOP), (rx, ry, SHOP)], (0, s, 0))))
    band = [(0.0, 0.0), (0.06, 0.0), (0.06, 0.04), (0.1, 0.06), (0.1, 0.12), (0.2, 0.16), (0.22, 0.2), (0.3, 0.26), (0.3, 0.33), (0.33, 0.35), (0.33, 0.38), (0.0, 0.38)]
    o = F.molding([(-0.07, -hw), (-0.07, hw)], band, 'front')
    o.data.transform(Matrix.Translation((0.0, 0.0, BAND - 0.38)))
    F.add(o)
    for yy in (-hw + 0.12, hw - 0.12):
        F.add(F.prism([(x - 0.07, z + BAND - 0.38) for x, z in console_profile(0.26, 0.62)], yy - 0.07, yy + 0.07, 'front'))

    def barber_board(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0x0d0f0e, rough=0.35)
        s.rect(18, 18, Wm - 36, Hm - 36, 0xb8914a, rough=0.3, metal=1.0)
        s.rect(26, 26, Wm - 52, Hm - 52, 0x0d0f0e, rough=0.35)
        outlined(s, 'FIFTH ST. BARBERS', Wm / 2, Hm * 0.36, 250, 0xd6ac58, 0x2a1a08, face='Black', tracking=1.12, metal=1.0, rough=0.25,
                 shadow=(0x000000, 10, -10))
        s.text('HAIRCUTS  ·  HOT TOWEL SHAVES  ·  SINCE 1961', Wm / 2, Hm * 0.12, 54, 0xcfc6b0, face='Bold', align='CENTER', tracking=1.35, rough=0.4)
        worn(s, Wm, Hm, 120, 0x3a3a36, 0.35, 11)

    def cleaners_board(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xe6e0d0, rough=0.5)
        s.rect(0, 0, Wm, 70, 0x9a2a28, rough=0.5)
        s.rect(0, Hm - 70, Wm, 70, 0x9a2a28, rough=0.5)
        s.text('NORTHSIDE', Wm / 2, Hm * 0.66, 70, 0x1f3c6e, face='Bold', align='CENTER', tracking=1.6, rough=0.5)
        squeezed(s, 'TAILOR & CLEANERS', Wm / 2, Hm * 0.24, 190, 0x1f3c6e, squeeze=0.92, face='Black', align='CENTER', tracking=1.05, rough=0.5)
        s.text('ALTERATIONS · SAME DAY', Wm * 0.05, Hm * 0.62, 40, 0x9a2a28, face='Bold', align='LEFT', rough=0.5)
        s.text('555-0163', Wm * 0.95, Hm * 0.62, 46, 0x9a2a28, face='Black', align='RIGHT', rough=0.5)
        worn(s, Wm, Hm, 260, 0x6a6458, 0.4, 5)

    def name_panel(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xa89c84, rough=0.75)
        s.text('THE CORDELIA', Wm / 2, Hm * 0.3, 150, 0x3a332a, face='Medium', align='CENTER', tracking=1.5, rough=0.8)
        worn(s, Wm, Hm, 50, 0x6a6050, 0.5, 2)

    F.add(F.sign('barber', 4.7, 0.52, barber_board, (-0.1, (BARB[0] + BARB[1]) / 2, SHOP + 0.3)))
    F.add(F.sign('cleaners', 4.7, 0.52, cleaners_board, (-0.1, (GATE[0] + GATE[1]) / 2, SHOP + 0.3)))
    F.add(F.sign('cordelia', 1.1, 0.3, name_panel, (-0.09, 0.0, SHOP + 0.32)))

    # Fifth St. Barbers, shut: the night light on over the chairs, the grille down.
    lamp, rm = (2.9, 4.6, 2.95), (6, 0.31, 1.05)
    sx0, sx1, sy0, sy1, sz1 = 0.3, 3.5, BARB[0] + 0.04, BARB[1] - 0.04, 3.2
    q = F.lquad
    F.add(q([(sx0, sy0, 0.02), (sx1, sy0, 0.02), (sx1, sy1, 0.02), (sx0, sy1, 0.02)], (0, 0, 1), 'checker', lamp, rm),
          q([(sx0, sy0, sz1), (sx1, sy0, sz1), (sx1, sy1, sz1), (sx0, sy1, sz1)], (0, 0, -1), 'ceil', lamp, rm),
          q([(sx1, sy0, 0.02), (sx1, sy1, 0.02), (sx1, sy1, sz1), (sx1, sy0, sz1)], (-1, 0, 0), 'subway', lamp, rm),
          q([(sx0, sy0, 0.02), (sx1, sy0, 0.02), (sx1, sy0, sz1), (sx0, sy0, sz1)], (0, 1, 0), 'subway', lamp, rm),
          q([(sx0, sy1, 0.02), (sx1, sy1, 0.02), (sx1, sy1, sz1), (sx0, sy1, sz1)], (0, -1, 0), 'subway', lamp, rm))
    for i in range(3):
        cy = sy0 + 1.0 + i * 1.45
        F.add(F.lblk(sx1 - 0.03, sx1, cy - 0.55, cy + 0.55, 1.05, 2.15, 'mirror', lamp, rm))
        F.add(F.lblk(sx1 - 0.5, sx1, cy - 0.65, cy + 0.65, 0.82, 0.88, 'counter', lamp, rm))
        F.add(F.lblk(sx1 - 0.5, sx1 - 0.02, cy - 0.65, cy + 0.65, 0.02, 0.82, 'furn', lamp, rm))
        F.add(F.lit_tag(parts.lathe_at('jar', [(0.0, 0.0), (0.05, 0.0), (0.05, 0.22), (0.035, 0.24), (0.0, 0.24)], Matrix.Translation((sx1 - 0.28, cy + 0.4, 0.88)), 12), 'jar', lamp, rm))
        # The chair: a chrome pedestal, red seat, back and headrest, arms, the footrest.
        cx = sx1 - 1.35
        F.add(F.lit_tag(parts.lathe_at('base', [(0.0, 0.02), (0.28, 0.02), (0.26, 0.08), (0.07, 0.12), (0.07, 0.42), (0.0, 0.42)], Matrix.Translation((cx, cy, 0.0)), 20), 'chrome', lamp, rm))
        F.add(F.lblk(cx - 0.3, cx + 0.3, cy - 0.3, cy + 0.3, 0.42, 0.6, 'vinyl', lamp, rm, 0.04))
        back = parts.rbox('back', (0, 0, 0), (0.12, 0.56, 0.62), 0.04, 2)
        back.data.transform(Matrix.Translation((cx - 0.32, cy, 0.92)) @ Matrix.Rotation(math.radians(12), 4, 'Y'))
        F.add(F.lit_tag(back, 'vinyl', lamp, rm))
        F.add(F.lblk(cx - 0.48, cx - 0.38, cy - 0.15, cy + 0.15, 1.28, 1.48, 'vinyl', lamp, rm, 0.03))
        for s in (-1, 1):
            F.add(F.lblk(cx - 0.25, cx + 0.28, cy + s * 0.34 - 0.04, cy + s * 0.34 + 0.04, 0.72, 0.78, 'chrome', lamp, rm, 0.015))
            F.add(F.lit_tag(parts.rod('armpost', (cx + 0.22, cy + s * 0.34, 0.6), (cx + 0.22, cy + s * 0.34, 0.72), 0.015, 8), 'chrome', lamp, rm))
        F.add(F.lblk(cx + 0.42, cx + 0.62, cy - 0.22, cy + 0.22, 0.18, 0.22, 'chrome', lamp, rm))
    # Waiting chairs down the left wall, a hairstyle chart over them, the fluorescent fittings (one on).
    for i in range(3):
        wy = sy1 - 0.3
        wx = 0.9 + i * 0.6
        F.add(F.lblk(wx - 0.24, wx + 0.24, wy - 0.24, wy + 0.2, 0.42, 0.5, 'vinyl', lamp, rm, 0.02))
        F.add(F.lblk(wx - 0.24, wx + 0.24, wy + 0.12, wy + 0.22, 0.5, 0.95, 'vinyl', lamp, rm, 0.02))
        F.add(F.lblk(wx - 0.22, wx + 0.22, wy - 0.2, wy + 0.18, 0.02, 0.42, 'chrome', lamp, rm))
    def styles(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xe9e1cc, rough=0.7)
        s.text('STYLES', Wm / 2, Hm - 95, 70, 0x8a1c1c, face='Black', align='CENTER', tracking=1.3, rough=0.7)
        r = random.Random(4)
        for i in range(6):
            cx, cy = 150 + (i % 3) * 300, Hm - 330 - (i // 3) * 300
            s.circle(cx, cy, 95, 0xd8c6a8, rough=0.7)
            s.circle(cx, cy + 40, 100, [0x2a1a10, 0x4a3018, 0x111111, 0x6a4a28, 0x1c1410, 0x3a2a1c][i], rough=0.7)
            s.circle(cx, cy - 20, 80, 0xd8b898, rough=0.7)
            s.rect(cx - 60, cy - 165, 120, 22, 0x8a1c1c, rough=0.7)
            s.text(['THE IVY', 'FLAT TOP', 'FADE', 'POMP', 'CREW', 'SIDE PART'][i], cx, cy - 210, 30, 0x222222, face='Bold', align='CENTER', rough=0.7)

    F.add(F.sign('styles', 0.95, 1.2, styles, (1.6, sy1 - 0.02, 1.85), normal=(0.0, -1.0, 0.0), depth=0.01, glow=0.9, px=512))
    for i, fx in enumerate((1.1, 2.2, 3.1)):
        F.add(F.lblk(fx - 0.55, fx + 0.55, 4.45, 4.75, sz1 - 0.06, sz1, 'fixture' if i == 2 else 'counter', lamp, rm))
    # The storefront: tiled bulkhead, the display window's frame and transom, the door.
    F.add(F.blk(0.14, 0.34, BARB[0], 5.0, 0.0, 0.52, 'tile'))
    F.add(F.blk(0.09, 0.34, BARB[0], 5.0, 0.52, 0.58, 'front', 0.008))
    for y in (BARB[0] + 0.04, 3.1, 4.96, BARB[1] - 0.04):
        F.add(F.blk(0.18, 0.3, y - 0.045, y + 0.045, 0.0 if y > 4.9 else 0.58, SHOP, 'front', 0.006))
    F.add(F.blk(0.16, 0.3, BARB[0], BARB[1], 2.6, 2.7, 'front', 0.006))
    win = (3.75, 2.7, 5.0, 0.6)
    for a, b in ((BARB[0] + 0.09, 3.055), (3.145, 4.915), (5.005, BARB[1] - 0.09)):
        F.add(F.pane(0.24, a, b, 2.7, SHOP - 0.02, win, 'paper' if a > 5 else 'dark', 0.4))
    # The door (its glass left out: the shop shows through), a CLOSED card hung inside it.
    dy0, dy1 = 5.01, BARB[1] - 0.09
    F.add(F.blk(0.22, 0.27, dy0, dy0 + 0.1, 0.0, 2.6, 'front', 0.005), F.blk(0.22, 0.27, dy1 - 0.1, dy1, 0.0, 2.6, 'front', 0.005),
          F.blk(0.22, 0.27, dy0 + 0.1, dy1 - 0.1, 0.0, 0.42, 'front', 0.005), F.blk(0.22, 0.27, dy0 + 0.1, dy1 - 0.1, 2.45, 2.6, 'front', 0.005),
          F.blk(0.2, 0.22, dy0 + 0.14, dy0 + 0.17, 0.95, 1.25, 'brass'))

    def closed_card(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xf1ece0, rough=0.7)
        s.rect(8, 8, Wm - 16, Hm - 16, 0xb5241f, rough=0.6)
        s.text('CLOSED', Wm / 2, Hm * 0.3, 92, 0xf1ece0, face='Black', align='CENTER', rough=0.6)

    F.add(F.sign('closed', 0.34, 0.17, closed_card, (0.3, (dy0 + dy1) / 2, 1.75), depth=0.004, px=512))
    F.add(F.rod((0.3, (dy0 + dy1) / 2 - 0.12, 1.83), (0.3, (dy0 + dy1) / 2, 2.0), 0.002, 'black', 4),
          F.rod((0.3, (dy0 + dy1) / 2 + 0.12, 1.83), (0.3, (dy0 + dy1) / 2, 2.0), 0.002, 'black', 4))
    # The roll-down grille: horizontal rods on vertical links, its guides and its box.
    gx = -0.045
    zz = 0.12
    while zz < 3.0:
        F.add(F.rod((gx, BARB[0] + 0.06, zz), (gx, BARB[1] - 0.06, zz), 0.0055, 'galv', 6))
        zz += 0.085
    yy = BARB[0] + 0.2
    while yy < BARB[1] - 0.1:
        F.add(F.blk(gx - 0.003, gx + 0.012, yy - 0.008, yy + 0.008, 0.07, 3.03, 'galv'))
        yy += 0.27
    F.add(F.blk(gx - 0.025, gx + 0.025, BARB[0] + 0.06, BARB[1] - 0.06, 0.0, 0.07, 'galv', 0.008))
    for y in (BARB[0] + 0.03, BARB[1] - 0.03):
        F.add(F.blk(-0.08, 0.0, y - 0.035, y + 0.035, 0.0, 3.05, 'galv', 0.006))
    F.add(F.blk(-0.3, 0.0, BARB[0], BARB[1], 3.03, 3.33, 'galv', 0.012))
    # The pole on its bracket, lit.
    py, px = 6.52, -0.36
    for z in (1.82, 2.48):
        F.add(F.blk(px, -0.14, py - 0.02, py + 0.02, z - 0.015, z + 0.015, 'steel'))
    F.add(F.blk(-0.16, -0.14, py - 0.07, py + 0.07, 1.75, 2.55, 'steel'))
    pole_rm = (6, 0.5, 1.0)
    F.add(F.lit_tag(parts.lathe_at('pole', [(0.0, 1.87), (0.07, 1.87), (0.07, 2.43), (0.0, 2.43)], Matrix.Translation((px, py, 0.0)), 24), 'pole', (px, py, 0.0), pole_rm))
    F.add(F.tag(parts.lathe_at('cap', [(0.0, 1.78), (0.06, 1.78), (0.09, 1.82), (0.09, 1.87), (0.0, 1.87)], Matrix.Translation((px, py, 0.0)), 24), 'steel'))
    F.add(F.tag(parts.lathe_at('cap', [(0.0, 2.43), (0.09, 2.43), (0.09, 2.48), (0.06, 2.52), (0.0, 2.52)], Matrix.Translation((px, py, 0.0)), 24), 'steel'))
    F.add(F.lit_tag(parts.lathe_at('globe', [(0.0, 2.52), (0.05, 2.53), (0.085, 2.6), (0.07, 2.67), (0.0, 2.69)], Matrix.Translation((px, py, 0.0)), 20), 'lantern', (px, py, 2.6), pole_rm))

    # The tenants' door, in a marble-lined recess, the hall beyond lit.
    ey = 0.62
    F.add(F.blk(-0.36, 0.95, -ey, ey, 0.0, 0.16, 'granite', 0.012))
    for s in (-1, 1):
        F.add(F.blk(F.wall_t, 0.95, s * ey - 0.03, s * ey + 0.03, 0.16, SHOP, 'marble'))
    F.add(F.blk(0.0, 0.95, -ey, ey, SHOP - 0.1, SHOP, 'marble'))
    F.add(F.blk(0.9, 0.97, -ey, -0.5, 0.16, SHOP - 0.1, 'door'), F.blk(0.9, 0.97, 0.5, ey, 0.16, SHOP - 0.1, 'door'),
          F.blk(0.9, 0.97, -0.5, 0.5, 2.38, 2.5, 'door'))
    # The door leaf: rails and stiles round its glass, raised panels below, brass.
    dx0, dx1 = 0.91, 0.96
    F.add(F.blk(dx0, dx1, -0.5, -0.38, 0.16, 2.38, 'door', 0.004), F.blk(dx0, dx1, 0.38, 0.5, 0.16, 2.38, 'door', 0.004),
          F.blk(dx0, dx1, -0.38, 0.38, 0.16, 1.05, 'door', 0.004), F.blk(dx0, dx1, -0.38, 0.38, 2.2, 2.38, 'door', 0.004))
    F.add(F.blk(dx0 - 0.015, dx0, -0.3, 0.3, 0.32, 0.92, 'door', 0.006), F.blk(dx0 - 0.004, dx0, -0.44, 0.44, 0.17, 0.3, 'brass'))
    F.add(F.rod((dx0, 0.4, 1.05), (dx0 - 0.07, 0.4, 1.05), 0.012, 'brass', 10), F.blk(dx0 - 0.008, dx0, 0.37, 0.43, 0.97, 1.2, 'brass'))
    hall_rm, hall_lamp = (7, 0.62, 1.15), (2.0, 0.0, 3.0)
    hx0, hx1, hz0, hz1 = 0.97, 3.4, 0.16, SHOP - 0.1
    F.add(q([(hx0, -ey, hz0), (hx1, -ey, hz0), (hx1, ey, hz0), (hx0, ey, hz0)], (0, 0, 1), 'hex', hall_lamp, hall_rm),
          q([(hx0, -ey, hz1), (hx1, -ey, hz1), (hx1, ey, hz1), (hx0, ey, hz1)], (0, 0, -1), 'ceil', hall_lamp, hall_rm),
          q([(hx1, -ey, hz0), (hx1, ey, hz0), (hx1, ey, hz1), (hx1, -ey, hz1)], (-1, 0, 0), 'wall', hall_lamp, hall_rm),
          q([(hx0, -ey, hz0), (hx1, -ey, hz0), (hx1, -ey, hz1), (hx0, -ey, hz1)], (0, 1, 0), 'wall', hall_lamp, hall_rm),
          q([(hx0, ey, hz0), (hx1, ey, hz0), (hx1, ey, hz1), (hx0, ey, hz1)], (0, -1, 0), 'wall', hall_lamp, hall_rm))
    for i in range(8):
        F.add(F.lblk(1.9 + i * 0.25, 3.4, -0.55, 0.15, hz0, hz0 + 0.19 * (i + 1), 'stair', hall_lamp, hall_rm))
    F.add(F.lit_tag(parts.rod('rail', (1.75, 0.18, 1.22), (3.4, 0.18, 2.6), 0.028, 8), 'oak', hall_lamp, hall_rm))
    for i in range(7):
        bx = 1.98 + i * 0.25
        F.add(F.lit_tag(parts.rod('baluster', (bx, 0.18, hz0 + 0.19 * (i + 1)), (bx, 0.18, 1.22 + (bx - 1.75) * (1.38 / 1.65)), 0.012, 4), 'oak', hall_lamp, hall_rm))
    F.add(F.lit_tag(parts.rod('newel', (1.75, 0.18, hz0), (1.75, 0.18, 1.2), 0.05, 4), 'oak', hall_lamp, hall_rm))
    F.add(F.lit_tag(parts.lathe_at('dome', [(0.0, -0.1), (0.12, -0.06), (0.14, 0.0), (0.0, 0.0)], Matrix.Translation((2.0, 0.0, hz1)), 16), 'fixture', hall_lamp, hall_rm))
    F.add(F.lquad([(0.95, -0.48, 2.5), (0.95, 0.48, 2.5), (0.95, 0.48, SHOP - 0.12), (0.95, -0.48, SHOP - 0.12)], (-1, 0, 0), 'glow', hall_lamp, hall_rm))
    F.add(F.blk(0.93, 0.95, -0.015, 0.015, 2.5, SHOP - 0.1, 'door'))
    # A jelly-jar light hung in the recess's soffit, the intercom on the pier.
    jx = 0.42
    F.add(F.blk(jx - 0.08, jx + 0.08, -0.08, 0.08, SHOP - 0.13, SHOP - 0.1, 'black'))
    F.add(F.lit_tag(parts.lathe_at('jar', [(0.0, -0.18), (0.05, -0.18), (0.065, -0.1), (0.065, 0.0), (0.0, 0.0)], Matrix.Translation((jx, 0.0, SHOP - 0.13)), 16),
                    'lantern', (jx, 0.0, SHOP - 0.25), (7, 0.5, 1.0)))
    for a in range(4):
        ang = a * math.pi / 2
        F.add(F.rod((jx + 0.07 * math.cos(ang), 0.07 * math.sin(ang), SHOP - 0.13), (jx + 0.07 * math.cos(ang), 0.07 * math.sin(ang), SHOP - 0.3), 0.004, 'black', 4))

    def intercom(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0x9ea3a8, rough=0.35, metal=1.0)
        s.rect(6, 6, Wm - 12, Hm - 12, 0xb7bcc1, rough=0.3, metal=1.0)
        for i in range(14):
            s.rect(30, Hm - 40 - i * 7, Wm - 60, 3, 0x2a2c2e, rough=0.5)
        names = ['1A  RIVERA', '1B  OSEI', '2A  NOWAK', '2B  LIU', '3A  PETROV', '3B  ', '4A  HASSAN', '4B  COHEN', '5A  DIAZ', '5B  SUPT']
        for i, nm in enumerate(names):
            y = Hm - 160 - i * 26
            s.rect(14, y - 4, Wm - 60, 20, 0xf0ece2, rough=0.6)
            s.text(nm, 18, y, 13, 0x1a1a1a, face='Bold', rough=0.6)
            s.circle(Wm - 26, y + 6, 8, 0x3a3020, rough=0.3, metal=1.0)

    # The intercom on the recess's marble lining by the door (facing +Y, read left to right toward the street).
    ix, iy = 0.62, -ey + 0.03 + 0.014
    F.add(F.sign('intercom', 0.2, 0.44, intercom, (ix, iy, 1.45), normal=(0.0, 1.0, 0.0), depth=0.014, px=512))
    for i in range(10):
        F.add(F.tag(parts.disc('button', (ix + 0.1 - 0.174, iy + 0.003, 1.23 + (440 - 160 - 26 * i + 6) / 1000), (0, 1, 0), 0.006, 0.008, 10), 'brass'))
    # The standpipe connection on the pier between the door and the cleaners', low, its plate over it.
    # A brass Siamese: a round escutcheon, the body out from the wall, two inlets angled down and out, their caps
    # ribbed for the wrench, a chain from each cap to the body.
    sy, sz = -0.91, 0.52

    def along(p, d):
        return Matrix.Translation(p) @ Vector((0.0, 0.0, 1.0)).rotation_difference(Vector(d).normalized()).to_matrix().to_4x4()

    F.add(F.tag(parts.disc('escutcheon', (-0.15, sy, sz), (1, 0, 0), 0.09, 0.02, 24), 'brass'))
    F.add(F.tag(parts.lathe_at('body', [(0.0, 0.0), (0.05, 0.0), (0.05, 0.015), (0.042, 0.03), (0.042, 0.075), (0.05, 0.09), (0.0, 0.1)],
                               along((-0.155, sy, sz), (-1, 0, 0)), 20), 'brass'))
    for s in (-1, 1):
        d = Vector((-0.75, s * 0.5, -0.43)).normalized()
        p0 = Vector((-0.235, sy, sz))
        F.add(F.tag(parts.lathe_at('inlet', [(0.0, 0.0), (0.034, 0.0), (0.034, 0.1), (0.0, 0.1)], along(p0, d), 14), 'brass'))
        cap = [(0.0, 0.0), (0.05, 0.0), (0.05, 0.012), (0.046, 0.016), (0.046, 0.052), (0.04, 0.06), (0.0, 0.062)]
        F.add(F.tag(parts.lathe_at('cap', cap, along(p0 + d * 0.1, d), 12), 'brass'))
        F.add(F.rod(p0 + d * 0.13 + Vector((0.0, 0.0, -0.045)), Vector((-0.2, sy + s * 0.03, sz - 0.07)), 0.004, 'iron', 4))

    def standpipe_plate(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xf3eee4, rough=0.45)
        s.rect(3, 3, Wm - 6, Hm - 6, 0xb02a24, rough=0.45)
        s.text('STANDPIPE', Wm / 2, Hm * 0.27, Hm * 0.5, 0xf3eee4, face='Black', align='CENTER', tracking=1.12, rough=0.45)
        worn(s, Wm, Hm, 18, 0x5a1410, 0.4, 8)

    # On the pilaster's panel, under the flutes (this pier's lower rosette is left off for it).
    F.add(F.sign('standpipe', 0.3, 0.1, standpipe_plate, (-0.172, sy, 0.75), depth=0.008, px=512))
    # The cleaners' gate: down and locked, in its guides, its box above.
    F.add(F.blk(-0.055, -0.03, GATE[0] + 0.06, GATE[1] - 0.06, 0.06, 3.03, 'gate'))
    gw = GATE[1] - GATE[0] - 0.12
    F.trim_planar.append((lambda f: f.normal.x < -0.9 and -0.06 < f.calc_center_median().x < -0.05 and GATE[0] < f.calc_center_median().y < GATE[1]
                          and f.calc_center_median().z > 0.07, ('-y', 'z', -(GATE[1] - 0.06), -(GATE[0] + 0.06), 0.06, 3.03), fit_rect(gw, 2.97, (0.0, 0.0, 0.52, 1.0), (2048, 2048))))
    F.trim_box = (0.0, F.trim_planar[-1][2][3] + 0.005, 1.0, 1.0)
    F.add(F.blk(-0.075, -0.02, GATE[0] + 0.06, GATE[1] - 0.06, 0.0, 0.07, 'galv', 0.008))
    for y in (GATE[0] + 0.035, GATE[1] - 0.035):
        F.add(F.blk(-0.085, 0.0, y - 0.035, y + 0.035, 0.0, 3.05, 'galv', 0.006))
    for y in (GATE[0] + 0.5, GATE[1] - 0.5):
        F.add(F.blk(-0.11, -0.075, y - 0.03, y + 0.03, 0.02, 0.09, 'steel', 0.006))
        F.add(F.tag(parts.annulus('shackle', 0.012, 0.018, -0.004, 0.004, 12, Matrix.Translation((-0.093, y, 0.1)) @ Matrix.Rotation(math.radians(90), 4, 'Y')), 'steel'))
    F.add(F.blk(-0.32, 0.0, GATE[0], GATE[1], 3.03, 3.33, 'galv', 0.012))

    # The fire escape down the right two bays, a banner on it, plants and a chair out on it.
    zs = [fz(k) + SILL - 0.17 for k in range(1, NF + 1)]
    ya, yb = BAYS[0] - 1.25, BAYS[1] + 1.25
    F.add(fire_escape(F, ya, yb, zs))

    def banner(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xf2efe6, rough=0.85)
        s.rect(0, Hm - 150, Wm, 150, 0xc8202c, rough=0.85)
        s.text('APT FOR RENT', Wm / 2, Hm - 118, 110, 0xffffff, face='Black', align='CENTER', tracking=1.15, rough=0.85)
        s.text('1 BR · HEAT INCL.', Wm / 2, Hm * 0.42, 70, 0x1a1a1a, face='Bold', align='CENTER', tracking=1.1, rough=0.85)
        s.text('555-0187', Wm / 2, Hm * 0.1, 105, 0xc8202c, face='Black', align='CENTER', rough=0.85)
        for x, y in ((40, Hm - 40), (Wm - 40, Hm - 40), (40, 40), (Wm - 40, 40)):
            s.circle(x, y, 16, 0x8a8f94, rough=0.3, metal=1.0)

    F.add(F.sign('banner', 1.5, 0.62, banner, (-1.175, (ya + yb) / 2 + 0.4, zs[1] + 0.58), depth=0.006))
    F.add(pot_plant(F, -0.7, ya + 0.35, zs[0]), pot_plant(F, -0.55, ya + 0.75, zs[0], 0.1), pot_plant(F, -0.8, yb - 0.4, zs[2], 0.11))
    F.add(F.blk(-0.5, -0.1, yb - 0.95, yb - 0.6, zs[3], zs[3] + 0.04, 'galv'), F.blk(-0.12, -0.08, yb - 0.95, yb - 0.6, zs[3], zs[3] + 0.8, 'galv'))
    # A leader down the right end, cables wandering across.
    F.add(downspout(F, -hw + 0.38, TOP - 0.05, knots=((BAND + 0.3, -0.09), (BAND - 0.1, -0.47), (BAND - 0.55, -0.47), (BAND - 0.9, -0.2))))
    F.add(cable(F, [(-0.02, -hw + 0.6, TOP - 0.45), (-0.02, -hw + 0.61, 9.0), (-0.025, -hw + 0.62, BAND + 0.15), (-0.03, -3.0, BAND + 0.12), (-0.03, 0.5, BAND + 0.16),
                    (-0.03, 2.0, BAND + 0.1), (-0.02, 2.62, fz(1) + SILL - 0.12)]))
    F.add(cable(F, [(-0.02, -hw + 0.66, TOP - 0.5), (-0.02, -hw + 0.67, BAND + 0.3), (-0.02, -hw + 0.68, BAND + 0.12)], 0.005))
    return F.finish()


# ------------------------------------------------------------------ SM_Facade_1812

def rustication(F, y0, y1, z0, courses, openings, x_face=-0.05, depth=None, kind='stone', block=1.15, plinth=None):
    """A rusticated base: courses of chamfered blocks (V-joints) from y0 to y1, stopping at the openings (their sides
    are the reveals); plinth = (height, kind) lays the first course in another stone, a little prouder."""
    d = depth if depth is not None else F.wall_t
    out = []
    z = z0
    for ci, ch in enumerate(courses):
        za, zb = z, z + ch
        k = plinth[1] if plinth and ci == 0 else kind
        xf = x_face - (0.03 if plinth and ci == 0 else 0.0)
        cuts = sorted((o[0], o[1]) for o in openings if o[2] < zb - 1e-4 and o[3] > za + 1e-4)
        spans, a = [], y0
        for c0, c1 in cuts:
            if c0 > a + 1e-4:
                spans.append((a, c0))
            a = max(a, c1)
        if a < y1 - 1e-4:
            spans.append((a, y1))
        off = 0.5 * block if ci % 2 else 0.0
        for a, b in spans:
            edges = [a]
            t = y0 - off
            while t < b:
                t += block
                if a + 0.3 < t < b - 0.3:
                    edges.append(t)
            edges.append(b)
            for e0, e1 in zip(edges, edges[1:]):
                out.append(F.blk(xf, d, e0, e1, za, zb, k, 0.022, 1))
        z = zb
    return out


def f1812():
    """1812 Fifth Street: a 1912 apartment house, buff brick over a rusticated limestone base."""
    F = Front('1812', 16.0, wall_t=0.4, seed=1812)
    W, hw = F.W, F.W / 2
    BASE = 3.6
    FH = 3.4

    def fz(k):
        return BASE + (k - 2) * FH          # storeys 2..4
    ROOF = fz(5)                            # 13.8
    CH = 0.8
    WALL_TOP = ROOF + CH + 0.5              # the parapet over the cornice
    BAYS = [-6.4, -3.2, 0.0, 3.2, 6.4]
    WW, WH, SILL = 1.3, 1.85, 0.75
    ENT = (-1.1, 1.1, 0.0, 2.7)
    GWIN = (1.35, 3.15)                     # the ground floor windows' sill and head

    palette = dict(stone=dict(type='stone', hex=0xc2b79d, soot=0.5), granite=dict(type='granite', hex=0x605e5a), sash=dict(type='paint', hex=0x1f3a2e, under=0x6a5a48, chips=0.5),
                   door=dict(type='paint', hex=0x1f3a2e, under=0x4a3020, chips=0.35, rough=0.32), casing=dict(type='paint', hex=0xd9d1bf, under=0x7a6a58, chips=0.4),
                   cornice=dict(type='paint', hex=0xcfc6b1, under=0x5d6266, chips=0.5, soot=0.3, under_metal=0.6), iron=dict(type='iron'), galv=dict(type='galv'),
                   brass=dict(type='brass'), black=dict(type='black'), steel=dict(type='steel'), ac=dict(type='plastic', hex=0xc9c2b2), tar=dict(type='tar'),
                   marble=dict(type='marble', hex=0xd6d0c4))
    F.setup(brick_material('ft_wall', 0xa8875e, 0x8c6c48, 0x8e877a, light=0xc4a47a, dark=0x5e4430, soot=0.45, bond='running'), palette,
            mailbox=(-0.28, 0.95), lit_strength=5.0)

    # Upper floors: which window is which.
    plan = {2: ['dark', 'warm', 'stair', 'shade', 'cool'], 3: ['tv', 'shade', 'stair', 'laptop', 'sheer'], 4: ['shade', 'drawn', 'stair', 'dark', 'warm']}
    acs = {(2, 0), (4, 3)}
    openings = []
    for k in (2, 3, 4):
        for i, y in enumerate(BAYS):
            w = 1.0 if y == 0.0 else WW
            z0 = fz(k) + SILL
            openings.append((y - w / 2, y + w / 2, z0, z0 + WH))
            seed = 0.37 if plan[k][i] == 'laptop' else F.rnd.random()
            F.add(window(F, y, z0, w, WH, plan[k][i], seed, fz(k), fz(k + 1) - 0.32, room_w=2.9, panes=(1, 1),
                         lintel='eared' if k == 2 else 'key', ac=(k, i) in acs, curtains=True if plan[k][i] == 'laptop' else None))
    wall = F.wall_part(wall_mesh('brick', -hw, hw, BASE, WALL_TOP, openings, F.wall_t, ends=ROOM_BACK))
    F.add(wall)
    F.layout_wall([(lambda f: f.normal.x < -0.9, ('-y', 'z', -hw, hw, BASE, WALL_TOP), W, WALL_TOP - BASE)], px=(4096, 2048), hw=hw)
    # The base's party-wall returns, under the brick's, from the back of the rusticated blocks (whose own ends
    # are the first 45 cm).
    for s in (-1, 1):
        ry, rx = s * (hw + 0.0005), F.wall_t - 0.05     # tucked under the blocks' chamfers, a hair proud of their ends
        F.add(F.wall_part(quad_obj('base_ret', [(rx, ry, 0.0), (ROOM_BACK, ry, 0.0), (ROOM_BACK, ry, BASE), (rx, ry, BASE)], (0, s, 0))))
    F.add(F.blk(0.05, ROOM_BACK, -hw + 0.01, hw - 0.01, WALL_TOP - 0.06, WALL_TOP - 0.02, 'tar'))
    F.back_plate(0.0, WALL_TOP)

    # Limestone: the base, rusticated, on a granite plinth; the belt course; quoins; the frieze band; coping.
    # The course over the door stops at the entrance too (the entablature hides the gap): its underside would lie
    # in the recess soffit's plane, its joint grooves showing through the marble.
    gopen = [(ENT[0], ENT[1], ENT[2], ENT[3] + 0.05)] + [(y - WW / 2, y + WW / 2, GWIN[0], GWIN[1]) for y in BAYS if y != 0.0]
    F.add(rustication(F, -hw, hw, 0.0, [0.45] * 8, gopen, plinth=(0.45, 'granite')))
    for y in BAYS:
        if y == 0.0:
            continue
        F.add(F.blk(-0.1, 0.04, y - 0.16, y + 0.16, GWIN[1] - 0.04, GWIN[1] + 0.45, "stone", 0.015))
    belt = [(0.0, 0.0), (0.06, 0.0), (0.06, 0.04), (0.1, 0.07), (0.1, 0.16), (0.14, 0.19), (0.14, 0.25), (0.0, 0.25)]
    o = F.molding([(-0.05, -hw), (-0.05, hw)], belt, 'stone')
    o.data.transform(Matrix.Translation((0.0, 0.0, BASE)))
    F.add(o)
    z = BASE + 0.25
    i = 0
    while z + 0.3 < ROOF:
        for s in (-1, 1):
            ln = 0.48 if i % 2 == 0 else 0.3
            F.add(F.blk(-0.03, 0.0, s * hw - s * ln, s * hw, z + 0.012, z + 0.3 - 0.012, 'stone', 0.012) if s > 0 else
                  F.blk(-0.03, 0.0, -hw, -hw + ln, z + 0.012, z + 0.3 - 0.012, 'stone', 0.012))
        z += 0.3
        i += 1
    F.add(F.blk(-0.035, 0.0, -hw, hw, ROOF - 0.5, ROOF - 0.36, 'stone', 0.006))
    F.add(cornice(F, [(0.0, -hw), (0.0, hw)], ROOF, CH, 0.62))
    F.add(F.blk(-0.07, F.wall_t + 0.03, -hw - 0.02, hw + 0.02, WALL_TOP, WALL_TOP + 0.1, 'stone', 0.01))

    # Ground floor flats: raised, behind iron guards; one lit through its blinds, one with a lamp on.
    glooks = {-6.4: 'shade', -3.2: 'blinds', 3.2: 'sheer', 6.4: 'warm'}
    for y in BAYS:
        if y == 0.0:
            continue
        F.add(window(F, y, GWIN[0], WW, GWIN[1] - GWIN[0], glooks[y], F.rnd.random(), 0.55, BASE - 0.3, room_w=2.9, lintel='none',
                     guard=True, depth=F.wall_t, curtains=True if glooks[y] == 'warm' else None))

    # The entrance: a stone surround (pilasters, entablature with the number), the recess lined in stone.
    for s in (-1, 1):
        y0, y1 = sorted((s * 1.1, s * 1.47))
        F.add(F.blk(-0.18, -0.04, y0, y1, 0.0, 0.42, 'granite', 0.01))
        F.add(F.blk(-0.15, -0.04, y0 + 0.02, y1 - 0.02, 0.42, 2.56, 'stone', 0.008))
        for yy in (y0 + 0.11, y1 - 0.11):
            F.add(F.blk(-0.16, -0.15, yy - 0.02, yy + 0.02, 0.6, 2.4, 'stone'))
        cap = [(0.0, 0.0), (0.03, 0.0), (0.03, 0.04), (0.06, 0.07), (0.07, 0.14), (0.0, 0.14)]
        o = F.molding([(-0.15, y0), (-0.15, y1)], cap, 'stone')
        o.data.transform(Matrix.Translation((0.0, 0.0, 2.56)))
        F.add(o)
    F.add(F.blk(-0.15, -0.04, -1.5, 1.5, 2.7, 2.86, 'stone', 0.008), F.blk(-0.12, -0.04, -1.47, 1.47, 2.86, 3.36, 'stone', 0.006))
    ent_c = [(0.0, 0.0), (0.04, 0.0), (0.04, 0.03), (0.08, 0.06), (0.1, 0.1), (0.24, 0.12), (0.26, 0.16), (0.26, 0.22), (0.0, 0.22)]
    o = F.molding([(-0.12, -1.56), (-0.12, 1.56)], ent_c, 'stone')
    o.data.transform(Matrix.Translation((0.0, 0.0, 3.36)))
    F.add(o)
    F.add(F.blk(-0.45, 0.7, -1.12, 1.12, 0.0, 0.14, 'granite', 0.012))
    for s in (-1, 1):
        F.add(F.blk(F.wall_t, 0.7, s * 1.1 - 0.02, s * 1.1 + 0.02, 0.14, 2.7, 'marble'))
    F.add(F.blk(-0.045, 0.7, -1.1, 1.1, 2.697, 2.74, 'marble'))
    for s in (-1, 1):
        y0, y1 = sorted((s * 0.68, s * 1.1))
        F.add(F.blk(0.68, 0.72, y0, y1, 0.14, 2.7, 'marble'))
    F.add(F.blk(0.68, 0.72, -0.68, 0.68, 2.62, 2.7, 'marble'))
    # Door 1812: casing, transom, the leaf with its glass and grille, brass.
    for s in (-1, 1):
        y0, y1 = sorted((s * 0.55, s * 0.68))
        F.add(F.blk(0.61, 0.7, y0, y1, 0.14, 2.62, 'casing', 0.006))
    F.add(F.blk(0.61, 0.7, -0.68, 0.68, 2.56, 2.66, 'casing', 0.006), F.blk(0.62, 0.7, -0.55, 0.55, 2.3, 2.36, 'casing', 0.004))
    dx0, dx1 = 0.63, 0.68
    F.add(F.blk(dx0, dx1, -0.55, -0.43, 0.14, 2.3, 'door', 0.004), F.blk(dx0, dx1, 0.43, 0.55, 0.14, 2.3, 'door', 0.004),
          F.blk(dx0, dx1, -0.43, 0.43, 0.14, 1.0, 'door', 0.004), F.blk(dx0, dx1, -0.43, 0.43, 2.14, 2.3, 'door', 0.004))
    for a, b in ((-0.38, -0.02), (0.02, 0.38)):
        F.add(F.blk(dx0 - 0.012, dx0, a, b, 0.44, 0.9, 'door', 0.008))
    F.add(F.blk(dx0 - 0.003, dx0, -0.53, 0.53, 0.14, 0.34, 'brass'))
    F.add(F.rod((dx0, 0.47, 1.02), (dx0 - 0.065, 0.47, 1.02), 0.011, 'brass', 10), F.rod((dx0 - 0.06, 0.47, 1.02), (dx0 - 0.06, 0.36, 1.0), 0.01, 'brass', 8),
          F.blk(dx0 - 0.006, dx0, 0.445, 0.495, 0.94, 1.24, 'brass', 0.004), F.tag(parts.disc('bolt', (dx0 - 0.004, 0.47, 1.17), (1, 0, 0), 0.018, 0.008, 16), 'brass'))
    # The grille over the door's glass: bars and a ring.
    gxx = dx0 - 0.012
    for yy in (-0.3, -0.15, 0.15, 0.3):
        F.add(F.blk(gxx - 0.006, gxx + 0.006, yy - 0.007, yy + 0.007, 1.0, 2.14, 'black'))
    F.add(F.blk(gxx - 0.006, gxx + 0.006, -0.43, 0.43, 1.56, 1.58, 'black'))
    ring = core.sweep('ring', [(gxx, 0.14 * math.cos(a * math.pi / 12), 1.57 + 0.14 * math.sin(a * math.pi / 12)) for a in range(24)], [(0.007, 0.007), (-0.007, 0.007), (-0.007, -0.007), (0.007, -0.007)], closed=True, caps=False)
    F.add(F.tag(ring, 'black'))

    def transom(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xffdcaa, rough=0.1)
        for x in (Wm * 0.25, Wm * 0.5, Wm * 0.75):
            s.rect(x - 4, 0, 8, Hm, 0x2a2014, rough=0.3)
        outlined(s, '1812', Wm / 2, Hm * 0.2, 130, 0xc89a3c, 0x1c1408, face='Black', tracking=1.25, width=6, metal=1.0, rough=0.22)

    F.add(F.sign('transom', 1.1, 0.2, transom, (0.655, 0.0, 2.46), depth=0.008, glow=1.0, backlit=True, px=1024))

    def plaque(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xb08a40, rough=0.25, metal=1.0)
        s.rect(10, 10, Wm - 20, Hm - 20, 0x17302a, rough=0.35)
        s.rect(18, 18, Wm - 36, Hm - 36, 0xa47f3a, rough=0.25, metal=1.0)
        s.rect(22, 22, Wm - 44, Hm - 44, 0x17302a, rough=0.35)
        outlined(s, '1812', Wm / 2, Hm * 0.2, 170, 0xd2a64e, 0x0c1a16, face='Black', tracking=1.18, width=5, metal=1.0, rough=0.2, shadow=(0x050a08, 5, -5))

    F.add(F.sign('number', 0.92, 0.26, plaque, (-0.135, 0.0, 3.11), depth=0.015))

    def buzzer(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0x8e9398, rough=0.3, metal=1.0)
        s.rect(8, 8, Wm - 16, Hm - 16, 0xaeb3b8, rough=0.28, metal=1.0)
        for i in range(9):
            s.rect(40, Hm - 60 - i * 9, Wm - 80, 4, 0x1e2022, rough=0.5)
        s.circle(Wm / 2, Hm - 150, 13, 0x0c0d0e, rough=0.1)
        s.circle(Wm / 2, Hm - 150, 6, 0x1e3048, rough=0.05)
        names = ['1A  SUPT', '1B  OYELARAN', '2A  KOVACS', '2B  REYES', '3A  ABERNATHY', '3B  ', '4A  MORENO', '4B  TANAKA']
        for i, nm in enumerate(names):
            y = Hm - 205 - i * 40
            s.rect(16, y - 6, Wm - 72, 26, 0xf2eee4, rough=0.6)
            s.text(nm, 22, y, 15, 0x1a1a1a, face='Bold', rough=0.6)
            s.circle(Wm - 30, y + 7, 13, 0x2a2a2a, rough=0.3, metal=1.0)
        s.text('1812', Wm / 2, 20, 26, 0x2a2c2e, face='Black', align='CENTER', tracking=1.4, rough=0.4)

    # Surface-mounted on the recess wall: its back on the rusticated reveal (y 1.1), its face clear of the marble
    # lining's 2 cm (which starts at x 0.4, under the panel's inner edge).
    by = 1.076
    F.add(F.sign('buzzer', 0.26, 0.6, buzzer, (0.33, by, 1.48), normal=(0.0, -1.0, 0.0), depth=1.1 - by, px=1024))
    for i in range(8):
        zz = 1.48 - 0.3 + (600 - 205 - i * 40 + 7) / 1000
        F.add(F.tag(parts.disc('button', (0.2 + 0.23, by - 0.003, zz), (0, 1, 0), 0.008, 0.01, 12), 'brass'))

    def supt(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xe9e4d8, rough=0.4)
        s.rect(6, 6, Wm - 12, Hm - 12, 0x1f3a2e, rough=0.4)
        s.text('SUPERINTENDENT', Wm / 2, Hm * 0.6, 20, 0xe9e4d8, face='Bold', align='CENTER', tracking=1.3, rough=0.4)
        s.text('RING 1A', Wm / 2, Hm * 0.2, 30, 0xe9e4d8, face='Black', align='CENTER', tracking=1.2, rough=0.4)

    F.add(F.sign('supt', 0.24, 0.12, supt, (-0.06, -1.85, 1.55), depth=0.011, px=512))
    # The lamps: a carriage lamp each side, a downlight in the recess soffit.
    for s in (-1, 1):
        F.add(lantern(F, -0.05, s * 1.85, 2.2))
    F.add(F.lit_tag(parts.lathe_at('downlight', [(0.0, 0.0), (0.075, 0.0), (0.075, 0.01), (0.0, 0.01)], Matrix.Translation((0.35, 0.0, 2.69)), 16), "lantern", (0.35, 0.0, 2.6), (7, 0.5, 1.0)))
    F.add(F.tag(parts.annulus('trim', 0.075, 0.095, 0.0, 0.012, 24, Matrix.Translation((0.35, 0.0, 2.688))), "black"))

    # The vestibule through the door: marble, hex tile, the brass mailboxes facing the street, the inner door.
    lamp, rm = (1.5, 0.0, 2.65), (7, 0.55, 1.15)
    vx0, vx1, vy0, vy1, vz0, vz1 = 0.7, 2.3, -0.95, 0.95, 0.14, 2.95
    q = F.lquad
    F.add(q([(vx0, vy0, vz0), (vx1, vy0, vz0), (vx1, vy1, vz0), (vx0, vy1, vz0)], (0, 0, 1), 'hex', lamp, rm),
          q([(vx0, vy0, vz1), (vx1, vy0, vz1), (vx1, vy1, vz1), (vx0, vy1, vz1)], (0, 0, -1), 'ceil', lamp, rm))
    for (pts, want) in (([(vx1, vy0, 0), (vx1, vy1, 0), (vx1, vy1, 1), (vx1, vy0, 1)], (-1, 0, 0)), ([(vx0, vy0, 0), (vx1, vy0, 0), (vx1, vy0, 1), (vx0, vy0, 1)], (0, 1, 0)),
                        ([(vx0, vy1, 0), (vx1, vy1, 0), (vx1, vy1, 1), (vx0, vy1, 1)], (0, -1, 0))):
        lower = [(x, y, vz0 if zz == 0 else 1.05) for x, y, zz in pts]
        upper = [(x, y, 1.05 if zz == 0 else vz1) for x, y, zz in pts]
        F.add(q(lower, want, 'marble', lamp, rm), q(upper, want, 'wall', lamp, rm))
    F.add(F.lblk(vx1 - 0.04, vx1, -0.3, 0.3, 0.93, 1.82, 'mailbox', lamp, rm))
    F.add(F.lblk(vx1 - 0.06, vx1, -0.32, 0.32, 0.9, 0.93, 'oak', lamp, rm), F.lblk(vx1 - 0.06, vx1, -0.32, 0.32, 1.82, 1.86, 'oak', lamp, rm))
    F.add(F.lblk(1.15, 2.05, vy1 - 0.05, vy1, vz0, 2.32, 'oak', lamp, rm), F.lblk(1.3, 1.9, vy1 - 0.055, vy1 - 0.05, 1.25, 2.12, 'glow', lamp, rm))
    F.add(F.lit_tag(parts.lathe_at('dome', [(0.0, -0.11), (0.14, -0.07), (0.16, 0.0), (0.0, 0.0)], Matrix.Translation((1.5, 0.0, vz1)), 16), 'fixture', lamp, rm))

    # A leader and the cables.
    F.add(downspout(F, -hw + 0.3, ROOF - 0.05, knots=((BASE + 0.45, -0.09), (BASE - 0.15, -0.17))))
    F.add(cable(F, [(-0.02, -hw + 0.55, ROOF - 0.6), (-0.02, -hw + 0.56, BASE + 0.4), (-0.03, -4.0, BASE + 0.32), (-0.03, -2.3, BASE + 0.36), (-0.02, -1.4, fz(2) + SILL - 0.1)]))
    return F.finish()


# ------------------------------------------------------------------ SM_Facade_Brownstone

def picket_fence(F, a, b, z0=0.0, h=1.05, pitch=0.11, posts=1.5):
    """A cast-iron areaway fence from a to b (x, y) on a low stone curb: square pickets with spear tips between rails,
    posts with ball finials."""
    out = []
    A, B = Vector((a[0], a[1], 0.0)), Vector((b[0], b[1], 0.0))
    L = (B - A).length
    d = (B - A).normalized()
    M = frame_matrix(A, (-d.y, d.x, 0.0))
    local = [F.blk(-0.11, 0.11, 0.0, L, z0, z0 + 0.16, 'brown', 0.01)]
    n = int(L / pitch)
    for i in range(n + 1):
        y = 0.03 + (L - 0.06) * i / n
        local.append(F.blk(-0.008, 0.008, y - 0.008, y + 0.008, z0 + 0.16, z0 + h, 'iron'))
        local.append(F.tag(parts.lathe_at('tip', [(0.0, 0.0), (0.016, 0.0), (0.0, 0.07)], Matrix.Translation((0.0, y, z0 + h)), 4), 'iron'))
    for zz in (z0 + 0.26, z0 + h - 0.08):
        local.append(F.blk(-0.012, 0.012, 0.0, L, zz - 0.012, zz + 0.012, 'iron'))
    m = max(1, int(L / posts))
    for i in range(m + 1):
        y = 0.04 + (L - 0.08) * i / m
        local.append(F.blk(-0.03, 0.03, y - 0.03, y + 0.03, z0 + 0.16, z0 + h + 0.08, 'iron', 0.004))
        local.append(F.tag(parts.lathe_at('ball', [(0.0, 0.0), (0.035, 0.01), (0.045, 0.05), (0.03, 0.09), (0.0, 0.1)], Matrix.Translation((0.0, y, z0 + h + 0.08)), 10), 'iron'))
    out += xform(local, M)
    return out


def brownstone():
    """A brownstone row house of the 1880s: high stoop, full-height three-sided bay, hooded windows, wood cornice."""
    F = Front('brownstone', 6.75, wall_t=0.36, seed=1887)
    W, hw = F.W, F.W / 2
    PARLOR = 1.4
    TOP, CH = 11.1, 0.9
    WALL_TOP = TOP + CH
    YC, HALF_W, HALF_F, PROJ = -1.15, 1.9, 1.1, 0.8
    DOOR = (1.55, 2.85)
    DY = (DOOR[0] + DOOR[1]) / 2
    LEVELS = {0: (0.0, PARLOR - 0.15, 0.38, 0.85), 1: (PARLOR, 4.9 - 0.3, 1.85, 2.3), 2: (4.9, 8.1 - 0.3, 5.55, 2.0), 3: (8.1, TOP - 0.3, 8.7, 1.75)}
    palette = dict(brown=dict(type='brown'), sash=dict(type='paint', hex=0x2b1e16, under=0x6b4a30, chips=0.55), wood=dict(type='wood', hex=0x4a2c18),
                   cornice=dict(type='paint', hex=0x2e251f, under=0x6a5440, chips=0.6, soot=0.7), iron=dict(type='iron'), brass=dict(type='brass'),
                   black=dict(type='black'), galv=dict(type='galv'), tar=dict(type='tar'), concrete=dict(type='concrete', hex=0x5d5c59), ac=dict(type='plastic', hex=0xc4bdad))
    F.setup(brownstone_material('bs_wall'), palette, lit_strength=5.0)

    # The bay: three faces, each a slab of its own with its windows (front: one wide window a floor; sides: narrow).
    faces = [('right', (-PROJ / 2, YC - (HALF_W + HALF_F) / 2, 0.0), (-0.70710678, -0.70710678, 0.0), PROJ * math.sqrt(2)),
             ('front', (-PROJ, YC, 0.0), (-1.0, 0.0, 0.0), 2 * HALF_F),
             ('left', (-PROJ / 2, YC + (HALF_W + HALF_F) / 2, 0.0), (-0.70710678, 0.70710678, 0.0), PROJ * math.sqrt(2))]
    looks = {(0, 'front'): 'blinds', (1, 'front'): 'drawn', (1, 'left'): 'drawn', (1, 'right'): 'drawn', (2, 'front'): 'shade', (2, 'left'): 'shade', (2, 'right'): 'shade',
             (3, 'front'): 'blinds', (3, 'left'): 'blinds', (3, 'right'): 'blinds'}
    seeds = {1: 0.21, 2: 0.62, 3: 0.45}
    for name, c, n, L in faces:
        M = frame_matrix(c, n)
        opens = []
        for lv, (fl, cl, z0, h) in LEVELS.items():
            if lv == 0 and name != 'front':
                continue
            w = 1.1 if name == 'front' and lv == 0 else (1.2 if name == 'front' else 0.6)
            opens.append((-w / 2, w / 2, z0, z0 + h))
            look = looks[(lv, name)]
            F.add(window(F, 0.0, z0, w, h, look, seeds.get(lv, F.rnd.random()), fl, cl, room_w=2.0, panes=(1, 1), lintel='hood' if name == 'front' and lv > 0 else ('flat' if lv > 0 else 'none'),
                         sill='none', guard=lv == 0, M=M, depth=0.3, stone='brown'))
        slab = wall_mesh('bay_' + name, -L / 2, L / 2, 0.0, WALL_TOP, opens, 0.3, ends=0)
        slab.data.transform(M)
        F.add(F.wall_part(slab))
    bm = bmesh.new()
    _face(bm, [bm.verts.new(p) for p in ((0.0, YC - HALF_W, WALL_TOP + 0.008), (-PROJ, YC - HALF_F, WALL_TOP + 0.008), (-PROJ, YC + HALF_F, WALL_TOP + 0.008), (0.0, YC + HALF_W, WALL_TOP + 0.008))], (0, 0, 1))
    F.add(F.tag(core.mesh_object('bay_roof', bm), 'tar'))

    # The main wall: the bay's notch, the door, the door bay's windows above it.
    openings = [(YC - HALF_W, YC + HALF_W, 0.0, WALL_TOP, False), (DOOR[0], DOOR[1], PARLOR - 0.06, 4.15)]  # the sill under the stoop's top
    for lv in (2, 3):
        fl, cl, z0, h = LEVELS[lv]
        openings.append((DY - 0.5, DY + 0.5, z0, z0 + h))
        F.add(window(F, DY, z0, 1.0, h, 'warm' if lv == 2 else 'dark', F.rnd.random(), fl, cl, room_w=1.9, panes=(1, 1), lintel='hood', sill='none', stone='brown'))
    F.add(F.wall_part(wall_mesh('wall', -hw, hw, 0.0, WALL_TOP, openings, F.wall_t, ends=ROOM_BACK)))
    F.layout_wall([(lambda f: f.normal.x < -0.9, ('-y', 'z', -hw, hw, 0.0, WALL_TOP), W, WALL_TOP)], hw=hw)
    F.add(F.blk(0.05, ROOM_BACK, -hw + 0.01, hw - 0.01, WALL_TOP - 0.06, WALL_TOP - 0.02, 'tar'))
    F.back_plate(0.0, WALL_TOP)

    # Moldings round the bay and across the front: a water table at the parlor floor, sill courses, the cornice.
    path = [(0.0, -hw), (0.0, YC - HALF_W), (-PROJ, YC - HALF_F), (-PROJ, YC + HALF_F), (0.0, YC + HALF_W), (0.0, hw)]
    course = [(0.0, 0.0), (0.05, 0.0), (0.07, 0.025), (0.07, 0.08), (0.09, 0.1), (0.09, 0.13), (0.0, 0.13)]
    for z, cut in ((PARLOR - 0.1, True), (LEVELS[1][2] - 0.13, True), (LEVELS[2][2] - 0.13, False), (LEVELS[3][2] - 0.13, False)):
        runs = [path[:-1] + [(0.0, DOOR[0] - 0.27)], [(0.0, DOOR[1] + 0.27), (0.0, hw)]] if cut else [path]
        for run in runs:
            o = F.molding(run, course, 'brown')
            o.data.transform(Matrix.Translation((0.0, 0.0, z)))
            F.add(o)
    F.add(cornice(F, path, TOP, CH, 0.55))

    # The door: pilasters, a hood on consoles, double doors of varnished oak with their glass, the stained-glass transom.
    for a, b in ((DOOR[0] - 0.25, DOOR[0] + 0.003), (DOOR[1] - 0.003, DOOR[1] + 0.25)):
        F.add(F.blk(-0.07, 0.02, a, b, PARLOR, 4.15, 'brown', 0.008))
    F.add(F.blk(-0.08, 0.05, DOOR[0] - 0.28, DOOR[1] + 0.28, 4.147, 4.36, 'brown', 0.008))
    hood = [(0.0, 0.0), (0.08, 0.0), (0.1, 0.03), (0.1, 0.1), (0.2, 0.13), (0.26, 0.2), (0.28, 0.24), (0.28, 0.3), (0.0, 0.3)]
    o = F.molding([(-0.08, DOOR[0] - 0.38), (-0.08, DOOR[1] + 0.38)], hood, 'brown')
    o.data.transform(Matrix.Translation((0.0, 0.0, 4.36)))
    F.add(o)
    for yy in (DOOR[0] - 0.14, DOOR[1] + 0.14):
        F.add(F.prism([(x - 0.07, z + 4.36) for x, z in console_profile(0.2, 0.62)], yy - 0.07, yy + 0.07, 'brown'))
    dx = 0.26
    F.add(F.blk(dx, dx + 0.1, DOOR[0], DOOR[0] + 0.07, PARLOR, 4.15, 'wood', 0.004), F.blk(dx, dx + 0.1, DOOR[1] - 0.07, DOOR[1], PARLOR, 4.15, 'wood', 0.004),
          F.blk(dx, dx + 0.1, DOOR[0] + 0.07, DOOR[1] - 0.07, 3.72, 3.8, 'wood', 0.004))
    for a, b in ((DOOR[0] + 0.07, DY), (DY, DOOR[1] - 0.07)):
        lx0, lx1 = dx + 0.02, dx + 0.07
        F.add(F.blk(lx0, lx1, a, a + 0.1, PARLOR, 3.72, 'wood', 0.004), F.blk(lx0, lx1, b - 0.1, b, PARLOR, 3.72, 'wood', 0.004),
              F.blk(lx0, lx1, a + 0.1, b - 0.1, PARLOR, 2.35, 'wood', 0.004), F.blk(lx0, lx1, a + 0.1, b - 0.1, 3.55, 3.72, 'wood', 0.004))
        F.add(F.blk(lx0 - 0.015, lx0, a + 0.16, b - 0.16, PARLOR + 0.2, 2.2, 'wood', 0.01))
        F.add(F.rod((lx0, b - 0.16 if a < DY else a + 0.16, 2.05), (lx0 - 0.06, b - 0.16 if a < DY else a + 0.16, 2.05), 0.018, 'brass', 10))
    F.add(F.blk(dx + 0.015, dx + 0.02, DY - 0.18, DY + 0.18, 1.85, 1.9, 'brass'))

    def stained(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0x2a2016, rough=0.4)
        cols = [0xe8c070, 0x8a2a2a, 0x2c5a7a, 0xd8d0b0, 0x3e6a3a]
        r = random.Random(9)
        nx = 9
        for i in range(nx):
            x0 = 10 + i * (Wm - 20) / nx
            for j in range(2):
                y0 = 10 + j * (Hm - 20) / 2
                s.rect(x0 + 5, y0 + 5, (Wm - 20) / nx - 10, (Hm - 20) / 2 - 10, cols[(i + j * 2 + (i // 3)) % len(cols)], rough=0.1)
        s.circle(Wm / 2, Hm / 2, Hm * 0.36, 0x2a2016, rough=0.4)
        s.circle(Wm / 2, Hm / 2, Hm * 0.32, 0xe8c070, rough=0.1)
        s.circle(Wm / 2, Hm / 2, Hm * 0.14, 0x8a2a2a, rough=0.1)

    F.add(F.sign('transom', DOOR[1] - DOOR[0] - 0.14, 0.33, stained, (dx + 0.05, DY, 3.8 + 0.175), depth=0.01, glow=1.0, backlit=False, px=512))
    lamp, rm = (1.3, DY, 3.6), (7, 0.4, 1.1)
    q = F.lquad
    vx0, vx1, vy0, vy1, vz0, vz1 = dx + 0.1, 2.2, DOOR[0], DOOR[1], PARLOR, 4.3
    F.add(q([(vx0, vy0, vz0), (vx1, vy0, vz0), (vx1, vy1, vz0), (vx0, vy1, vz0)], (0, 0, 1), 'hex', lamp, rm),
          q([(vx0, vy0, vz1), (vx1, vy0, vz1), (vx1, vy1, vz1), (vx0, vy1, vz1)], (0, 0, -1), 'ceil', lamp, rm),
          q([(vx1, vy0, vz0), (vx1, vy1, vz0), (vx1, vy1, vz1), (vx1, vy0, vz1)], (-1, 0, 0), 'oak', lamp, rm),
          q([(vx0, vy0, vz0), (vx1, vy0, vz0), (vx1, vy0, vz1), (vx0, vy0, vz1)], (0, 1, 0), 'wall', lamp, rm),
          q([(vx0, vy1, vz0), (vx1, vy1, vz0), (vx1, vy1, vz1), (vx0, vy1, vz1)], (0, -1, 0), 'wall', lamp, rm))
    F.add(F.lblk(vx1 - 0.04, vx1, DY - 0.35, DY + 0.35, PARLOR + 0.9, PARLOR + 2.3, 'glow', lamp, rm))
    F.add(F.lit_tag(parts.lathe_at('pendant', [(0.0, -0.25), (0.11, -0.2), (0.13, -0.05), (0.03, 0.0), (0.0, 0.0)], Matrix.Translation((1.3, DY, 3.75)), 12), 'shade', lamp, rm))

    # The stoop: seven risers up to the parlor floor between brownstone cheek walls, iron rails on them, newels.
    steps = [(0.36, 0.0), (0.36, PARLOR), (-0.3, PARLOR)]
    x, z = -0.3, PARLOR
    for i in range(7):
        z -= 0.2
        steps.append((x, z))
        if z > 0.01:
            x -= 0.27
            steps.append((x, z))
    F.add(F.prism(steps, DOOR[0] - 0.02, DOOR[1] + 0.02, 'brown'))
    x, z = -0.3, PARLOR
    for i in range(7):
        F.add(F.blk(x - 0.035, x + 0.25, DOOR[0] - 0.02, DOOR[1] + 0.02, z - 0.04, z + 0.006, 'brown', 0.01))
        z -= 0.2
        x -= 0.27
    slope = (PARLOR - 0.2) / (1.92 - 0.3)
    for ya, yb in ((DOOR[0] - 0.22, DOOR[0] - 0.02), (DOOR[1] + 0.02, DOOR[1] + 0.22)):
        F.add(F.prism([(0.0, 0.0), (0.0, PARLOR + 0.24), (-0.3, PARLOR + 0.24), (-1.95, PARLOR + 0.24 - 1.65 * slope), (-2.05, PARLOR + 0.24 - 1.65 * slope), (-2.05, 0.0)], ya, yb, 'brown'))
        yc = (ya + yb) / 2
        rail0, rail1 = (-0.25, PARLOR + 1.12), (-1.88, PARLOR + 1.12 - 1.63 * slope)
        F.add(F.rod((0.0, yc, rail0[1]), (rail0[0], yc, rail0[1]), 0.022, 'iron', 8), F.rod((rail0[0], yc, rail0[1]), (rail1[0], yc, rail1[1]), 0.022, 'iron', 8))
        bx = -0.08
        while bx > -1.82:
            zb = PARLOR + 0.24 - max(0.0, (-0.3 - bx)) * slope
            zt = rail0[1] - max(0.0, (rail0[0] - bx)) * slope
            F.add(F.blk(bx - 0.008, bx + 0.008, yc - 0.008, yc + 0.008, zb, zt, 'iron'))
            bx -= 0.12
        newel = [(0.0, 0.0), (0.07, 0.0), (0.07, 0.06), (0.045, 0.1), (0.04, 0.55), (0.065, 0.62), (0.075, 0.7), (0.05, 0.76), (0.06, 0.82), (0.0, 0.9)]
        F.add(F.tag(parts.lathe_at('newel', newel, Matrix.Translation((-1.98, yc, PARLOR + 0.24 - 1.65 * slope)), 12), 'iron'))
    # The areaway: an iron fence on a curb round the garden window, a gate by the stoop, bluestone inside.
    fx = -1.85
    F.add(picket_fence(F, (fx, -hw + 0.06), (fx, DOOR[0] - 0.3)))
    F.add(picket_fence(F, (0.0, -hw + 0.06), (fx, -hw + 0.06)))
    F.add(F.blk(fx + 0.1, 0.0, -hw + 0.06, DOOR[0] - 0.22, 0.0, 0.02, 'concrete'))

    def dog(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xd8b23a, rough=0.5)
        s.rect(6, 6, Wm - 12, Hm - 12, 0x151515, rough=0.5)
        s.rect(10, 10, Wm - 20, Hm - 20, 0xd8b23a, rough=0.5)
        s.text('BEWARE OF DOG', Wm / 2, Hm * 0.32, 30, 0x151515, face='Black', align='CENTER', tracking=1.1, rough=0.5)

    F.add(F.sign('dog', 0.26, 0.11, dog, (fx - 0.025, 0.6, 0.75), depth=0.004, px=512))
    F.add(downspout(F, -hw + 0.12, TOP - 0.5, x_off=-0.15))
    return F.finish()


# ------------------------------------------------------------------ SM_Facade_StoreUpper

def store_upper():
    """The three floors over the Lucky Penny at the corner of Market: a painted-brick block sitting on the store."""
    F = Front('storeupper', 12.9, wall_t=0.36, seed=212)
    W, hw = F.W, F.W / 2
    D = 14.2                  # the store's depth: the block's back wall
    Z0, BELT = 4.76, 5.4      # the store's roof line; the second floor
    FH = 3.1

    def fz(k):
        return BELT + (k - 2) * FH
    ROOF = fz(5)
    CH = 0.7
    WALL_TOP = ROOF + CH + 0.55
    FB = [-4.65, -1.55, 1.55, 4.65]
    SB = [1.6, 4.6, 7.6]
    WW, WH, SILL = 1.2, 1.8, 0.8
    GHOST = (9.3, 14.0, 6.3, 14.0)

    def ghost_art(s, Wm, Hm):
        # The older ad under it first (a hotel's), then the cigar maker's over it.
        for i, ch in enumerate('HOTEL'):
            s.text(ch, Wm * 0.82, Hm - 900 - i * 1150, 900, 0xe8e0cc, face='Black', align='CENTER', rough=0.8, alpha=0.4)
        s.rect(250, 250, Wm - 500, Hm - 500, 0xe6d9bc, rough=0.8)
        s.rect(380, 380, Wm - 760, Hm - 760, 0x6e2418, rough=0.8)
        squeezed(s, 'ROYAL', Wm / 2, Hm - 1500, 1050, 0xe6d9bc, squeeze=0.9, face='Black', align='CENTER', rough=0.8)
        squeezed(s, 'FLUSH', Wm / 2, Hm - 2550, 1050, 0xd9ac4a, squeeze=0.9, face='Black', align='CENTER', rough=0.8)
        # A royal flush in spades, fanned from a point under it as a hand holds them, the ace on top.
        px_, py_ = Wm / 2, Hm - 4750
        cw, ch = 640, 900
        for i, rank in enumerate(['10', 'J', 'Q', 'K', 'A']):
            a = (2 - i) * 0.27
            ca, sa = math.cos(a), math.sin(a)

            def at(lx, ly, ca=ca, sa=sa, cx=px_ - 760 * sa, cy=py_ + 760 * ca):
                return (cx + lx * ca - ly * sa, cy + lx * sa + ly * ca)

            for (w_, h_, colr) in ((cw + 40, ch + 40, 0x1a1412), (cw, ch, 0xe6d9bc)):
                x_, y_ = at(-w_ / 2, -h_ / 2)
                s.rect(x_, y_, w_, h_, colr, rough=0.8, rot=a)
            for lx, ly, sz in ((-cw / 2 + 105, ch / 2 - 190, 150), (cw / 2 - 105, -ch / 2 + 50, 150)):
                tx, ty = at(lx, ly)
                s.text(rank, tx, ty, sz, 0x1a1412, face='Black', align='CENTER', rough=0.8, rot=a if ly > 0 else a + math.pi)
            # The pip: a spade (two lobes, a point, a flared stem).
            S = 300 if rank != 'A' else 420
            for lx in (-0.27 * S, 0.27 * S):
                cx_, cy_ = at(lx, -0.02 * S + 40)
                s.circle(cx_, cy_, 0.3 * S, 0x1a1412, rough=0.8, n=40)
            s.poly([at(-0.56 * S, 0.02 * S + 40), at(0.56 * S, 0.02 * S + 40), at(0.0, 0.66 * S + 40)], 0x1a1412, rough=0.8)
            s.poly([at(-0.06 * S, -0.2 * S + 40), at(0.06 * S, -0.2 * S + 40), at(0.22 * S, -0.62 * S + 40), at(-0.22 * S, -0.62 * S + 40)], 0x1a1412, rough=0.8)
        s.text('CIGARS', Wm / 2, Hm - 5600, 760, 0xe6d9bc, face='Black', align='CENTER', tracking=1.05, rough=0.8)
        s.text('5¢', Wm / 2, 1100, 1100, 0xd9ac4a, face='Black', align='CENTER', rough=0.8)
        s.text('ALWAYS A WINNER', Wm / 2, 650, 300, 0xe6d9bc, face='Bold', align='CENTER', tracking=1.15, rough=0.8)

    gs = Sheet((GHOST[1] - GHOST[0]) * 1000.0, (GHOST[3] - GHOST[2]) * 1000.0)
    ghost_art(gs, (GHOST[1] - GHOST[0]) * 1000.0, (GHOST[3] - GHOST[2]) * 1000.0)
    gpaths = gs.render('fac_su_ghost', 1024)
    palette = dict(stone=dict(type='stone', hex=0xa9a192, soot=0.7), sash=dict(type='alu', hex=0xdcdad4, metal=0.2), cornice=dict(type='paint', hex=0x2c2a27, under=0x5d6266, chips=0.5, soot=0.6, under_metal=0.6),
                   iron=dict(type='iron'), grate=dict(type='grate'), galv=dict(type='galv'), black=dict(type='black'), ac=dict(type='plastic', hex=0xcbc4b4), tar=dict(type='tar'),
                   cedar=dict(type='wood', hex=0x6a5c4c), steel=dict(type='steel'), pot=dict(type='paint', hex=0x9a5234, under=0x6a3a24, chips=0.3), leaf=dict(type='leaf'))
    F.setup(brick_material('su_wall', 0x7a3a2c, 0x5c2c22, 0x7a7466, light=0x9a6450, dark=0x3a1d17, soot=0.5, paint=(0x8f897c, 0.55),
                           ghost=dict(paths=gpaths, a0=-GHOST[1], a1=-GHOST[0], z0=GHOST[2], z1=GHOST[3], n=(0.0, 1.0, 0.0), fade=0.78)), palette, lit_strength=5.0)
    F.cams += [(x, y, z) for x in (-2.0, 3.0, 8.0, 14.0, 22.0, 32.0) for y in (7.0, 9.5, 13.0, 20.0) for z in (1.7, 8.0, 20.0)] + [(22.0, -3.0, 22.0), (32.0, 0.0, 26.0), (8.0, -20.0, 18.0)]
    M_side = frame_matrix((D / 2, hw, 0.0), (0.0, 1.0, 0.0))
    M_back = frame_matrix((D, 0.0, 0.0), (1.0, 0.0, 0.0))
    M_south = frame_matrix((D / 2, -hw, 0.0), (0.0, -1.0, 0.0))

    # Windows: the front, then down Market. Gold leaf on the tax office's two; a FOR RENT card in one.
    front_plan = {2: ['blinds', 'shade', 'dark', 'dark'], 3: ['warm', 'dark', 'tv', 'sheer'], 4: ['shade', 'cool', 'dark', 'red']}
    side_plan = {2: ['dark', 'shade', 'dark'], 3: ['shade', 'warm', 'dark'], 4: ['drawn', 'dark', 'shade']}
    acs = {(3, 3, 'f'), (4, 2, 'f'), (2, 2, 's')}   # not (3, 1): that flat's empty, its FOR RENT card in the window
    fopen, sopen = [], []
    for k in (2, 3, 4):
        z0 = fz(k) + SILL
        for i, y in enumerate(FB):
            fopen.append((y - WW / 2, y + WW / 2, z0, z0 + WH))
            F.add(window(F, y, z0, WW, WH, front_plan[k][i], F.rnd.random(), fz(k), fz(k + 1) - 0.3, room_w=2.6, panes=(1, 1), lintel='flat',
                         ac=(k, i, 'f') in acs))
        for i, x in enumerate(SB):
            ly = x - D / 2
            sopen.append((ly - WW / 2, ly + WW / 2, z0, z0 + WH))
            F.add(window(F, ly, z0, WW, WH, side_plan[k][i], F.rnd.random(), fz(k), fz(k + 1) - 0.3, room_w=2.6, panes=(1, 1), lintel='flat',
                         ac=(k, i, 's') in acs, M=M_side))

    def office(lines):
        def draw(s, Wm, Hm):
            s.rect(0, 0, Wm, Hm, 0x0a0c0f, rough=0.05)
            y = Hm * 0.62
            for text, size, face in lines:
                outlined(s, text, Wm / 2, y, size, 0xd4a84e, 0x1a1206, face=face, width=size * 0.05, metal=1.0, rough=0.22, tracking=1.1)
                y -= size * 1.35
        return draw
    for y, lines in ((4.65, [('J. KOWALSKI', 120, 'Black'), ('INCOME TAX', 92, 'Bold'), ('NOTARY PUBLIC', 70, 'Bold')]),
                     (1.55, [('TAX RETURNS', 110, 'Black'), ('BOOKKEEPING', 80, 'Bold'), ('2ND FLOOR  ·  RING 2A', 56, 'Bold')])):
        z0 = fz(2) + SILL
        F.add(F.sign('office%d' % int(y), 0.99, 0.78, office(lines), (0.185, y, z0 + 0.511), depth=0.003, px=1024))

    def for_rent(s, Wm, Hm):
        s.rect(0, 0, Wm, Hm, 0xf2a21c, rough=0.6)
        s.rect(14, 14, Wm - 28, Hm - 28, 0x111111, rough=0.6)
        s.rect(20, 20, Wm - 40, Hm - 40, 0xf2a21c, rough=0.6)
        s.text('FOR RENT', Wm / 2, Hm * 0.55, 92, 0x111111, face='Black', align='CENTER', rough=0.6)
        s.text('555-0118', Wm / 2, Hm * 0.18, 60, 0xb01818, face='Black', align='CENTER', rough=0.6)

    F.add(F.sign('forrent', 0.45, 0.3, for_rent, (0.185, -1.55, fz(3) + SILL + 0.45), depth=0.003, px=512))

    # The walls: front and Market painted brick with their windows, the back and the party wall plain.
    front = wall_mesh('front', -hw, hw, Z0, WALL_TOP, fopen, F.wall_t, ends=0)
    side = wall_mesh('side', -D / 2, D / 2, Z0, WALL_TOP, sopen, F.wall_t, ends=0)
    side.data.transform(M_side)
    back = wall_mesh('back', -hw, hw, Z0, WALL_TOP, [], F.wall_t, ends=0)
    back.data.transform(M_back)
    south = wall_mesh('south', -D / 2, D / 2, Z0, WALL_TOP, [], F.wall_t, ends=0)
    south.data.transform(M_south)
    F.add(F.wall_part(front), F.wall_part(side), F.wall_part(back), F.wall_part(south))
    F.layout_wall([(lambda f: f.normal.x < -0.9, ('-y', 'z', -hw, hw, Z0, WALL_TOP), W, WALL_TOP - Z0),
                   (lambda f: f.normal.y > 0.9 and f.calc_center_median().y > hw - 0.05, ('x', 'z', 0.0, D, Z0, WALL_TOP), D, WALL_TOP - Z0)],
                  px=(4096, 2048), region=(0.0, 0.0, 1.0, 0.86),
                  rest=lambda f: (f.normal.x > 0.9 and f.calc_center_median().x > D - 0.4) or (f.normal.y < -0.9 and f.calc_center_median().y < -hw + 0.4))
    bm = bmesh.new()
    _face(bm, [bm.verts.new(p_) for p_ in ((0.0, -hw, Z0), (D, -hw, Z0), (D, hw, Z0), (0.0, hw, Z0))], (0, 0, -1))
    F.add(F.wall_part(core.mesh_object('underside', bm)))
    F.collapse.append(lambda f: f.normal.z < -0.9 and abs(f.calc_center_median().z - Z0) < 0.01)
    # The roof inside the parapet.
    F.add(F.blk(F.wall_t, D - F.wall_t, -hw + F.wall_t, hw - F.wall_t, ROOF + 0.18, ROOF + 0.22, 'tar'))

    # Cast stone: the band over the store (it hides the store's roof edge), quoins, lintels; the cornice; coping.
    band = [(0.0, 0.0), (0.14, 0.0), (0.14, 0.42), (0.19, 0.46), (0.24, 0.52), (0.29, 0.56), (0.29, 0.64), (0.0, 0.64)]
    o = F.molding([(0.0, -hw), (0.0, hw), (D, hw)], band, 'stone')
    o.data.transform(Matrix.Translation((0.0, 0.0, Z0)))
    F.add(o)
    z = BELT + 0.1
    i = 0
    while z + 0.32 < ROOF:
        ln = 0.5 if i % 2 == 0 else 0.32
        # The corner's quoin is one stone turning the corner (an L in plan), long on one face, short on the other.
        side = 0.32 if i % 2 == 0 else 0.5
        ql = [(0.08, hw - ln), (0.08, hw - 0.08), (side, hw - 0.08), (side, hw + 0.03), (-0.03, hw + 0.03), (-0.03, hw - ln)]
        qb = core.slab(ql, z + 0.012, z + 0.32 - 0.012)
        core.bevel(qb, lambda e: True, 0.012, 1)
        F.add(F.tag(core.mesh_object('quoin', qb), 'stone'))
        F.add(F.blk(-0.03, 0.08, -hw - 0.03, -hw + ln, z + 0.012, z + 0.32 - 0.012, 'stone', 0.012))
        F.add(F.blk(D - ln, D + 0.03, hw - 0.08, hw + 0.03, z + 0.012, z + 0.32 - 0.012, 'stone', 0.012))
        z += 0.32
        i += 1
    F.add(cornice(F, [(0.0, -hw), (0.0, hw), (D, hw)], ROOF, CH, 0.5))
    cope = [(-F.wall_t - 0.04, 0.0), (0.05, 0.0), (0.05, 0.1), (-F.wall_t - 0.04, 0.1)]
    o = F.molding([(0.0, 0.0), (0.0, hw), (D, hw), (D, -hw), (0.0, -hw), (0.0, -0.001)], cope, 'stone')
    o.data.transform(Matrix.Translation((0.0, 0.0, WALL_TOP)))
    F.add(o)

    # Down Market: the fire escape, a leader; on the roof the water tank.
    zs = [fz(k) + SILL - 0.17 for k in (2, 3, 4)]
    F.add(fire_escape(F, SB[1] - D / 2 - 1.4, SB[2] - D / 2 + 1.4, zs, M=M_side))
    pp = pot_plant(F, -0.7, SB[2] - D / 2 + 0.9, zs[1])
    F.add(xform(pp, M_side))
    F.add(downspout(F, D / 2 - 0.35, ROOF - 0.05, knots=((BELT + 0.5, -0.09), (BELT - 0.25, -0.36)), M=M_side))
    tx, ty, tz = 5.2, -3.0, ROOF + 0.22
    legs = 3.2
    for a in range(4):
        ang = math.pi / 4 + a * math.pi / 2
        lx, ly = tx + 1.1 * math.cos(ang), ty + 1.1 * math.sin(ang)
        F.add(F.blk(lx - 0.06, lx + 0.06, ly - 0.06, ly + 0.06, tz, tz + legs, 'iron'))
        F.add(F.rod((lx, ly, tz + 0.2), (tx + 1.1 * math.cos(ang + math.pi / 2), ty + 1.1 * math.sin(ang + math.pi / 2), tz + legs - 0.2), 0.014, 'iron', 4))
    for yy in (ty - 0.6, ty, ty + 0.6):
        F.add(F.blk(tx - 1.35, tx + 1.35, yy - 0.08, yy + 0.08, tz + legs - 0.2, tz + legs, 'iron'))
    tank = [(0.0, 0.0), (1.42, 0.0), (1.42, 0.06), (1.38, 0.1), (1.32, 2.9), (1.36, 2.95), (0.0, 2.95)]
    F.add(F.tag(parts.lathe_at('tank', tank, Matrix.Translation((tx, ty, tz + legs)), 40), 'cedar'))
    F.add(F.tag(parts.lathe_at('roof', [(0.0, 0.0), (1.5, 0.0), (1.5, 0.05), (0.0, 1.0)], Matrix.Translation((tx, ty, tz + legs + 2.95)), 40), 'tar'))
    F.add(F.tag(parts.lathe_at('finial', [(0.0, 0.0), (0.06, 0.0), (0.03, 0.25), (0.0, 0.3)], Matrix.Translation((tx, ty, tz + legs + 3.9)), 8), 'iron'))
    for h in (0.25, 0.75, 1.25, 1.75, 2.25, 2.7):
        r = 1.42 - (h / 2.9) * 0.1 + 0.012
        hoop = core.sweep('hoop', [(tx + r * math.cos(a * math.pi / 20), ty + r * math.sin(a * math.pi / 20), tz + legs + h) for a in range(40)],
                          [(0.008, 0.02), (-0.008, 0.02), (-0.008, -0.02), (0.008, -0.02)], closed=True, caps=False)
        F.add(F.tag(hoop, 'iron'))
    for sd in (-0.2, 0.2):
        F.add(F.blk(tx - 1.5, tx - 1.46, ty + sd - 0.02, ty + sd + 0.02, tz, tz + legs + 3.0, 'iron'))
    for j in range(int((legs + 3.0) / 0.3)):
        F.add(F.rod((tx - 1.48, ty - 0.2, tz + 0.3 + j * 0.3), (tx - 1.48, ty + 0.2, tz + 0.3 + j * 0.3), 0.01, 'iron', 4))
    return F.finish()


# ------------------------------------------------------------------ build

BUILDERS = {'walkup': walkup, 'brownstone': brownstone, '1812': f1812, 'storeupper': store_upper}


def build():
    core.reset()
    return [BUILDERS[k]() for k in WHICH if k in BUILDERS]


def _stem(img):
    return os.path.splitext(os.path.basename(img.filepath or img.name))[0]


def assemble(objs):
    """After the bake: a texture that came out one flat value over everything its UVs cover (the lit interiors' base
    color and normal, the prints' normal, which no bump touches) is exported as a 4x4 of that value instead of a
    1-2k image of it. The lit interiors' ORM goes the same way: under their emission the occlusion and roughness of a
    near-black base color change nothing. Saves about 11 MB of GPU memory per 2k lit slot, and the .glb's size."""
    for o in objs:
        for slot in o.material_slots:
            nodes = [n for n in slot.material.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image is not None]
            emissive = any(_stem(n.image).endswith('_Emissive') for n in nodes)
            stats = {}
            for n in nodes:
                img = n.image
                w, h = img.size
                if w * h <= 64:
                    continue
                px = np.empty(w * h * 4, dtype=np.float32)
                img.pixels.foreach_get(px)
                px = px.reshape(-1, 4)[:, :3]
                cov = px[px.sum(axis=1) > 1e-4]          # what the bake wrote (the rest is the cleared background)
                if len(cov) == 0:
                    continue
                med = np.median(cov, axis=0)
                flat = np.mean(np.abs(cov - med).max(axis=1) < 4.0 / 255.0) > 0.995
                stats[_stem(img)] = (img, cov.mean(axis=0), flat)
            dark_base = any(k.endswith('_BaseColor') and f and v.max() < 0.1 for k, (_, v, f) in stats.items())
            for key, (img, mean, flat) in stats.items():
                if key.endswith('_ORM') and emissive and dark_base:
                    flat, mean = True, np.array([1.0, mean[1], mean[2]], dtype=np.float32)
                if not flat or key.endswith('_Emissive'):
                    continue
                img.scale(4, 4)
                img.pixels.foreach_set(np.tile(np.append(mean, 1.0).astype(np.float32), 16))
                img.save()
                print(f'[facades] {key}: flat, exported as 4x4')
    return objs


def pose_for_review(objs):
    """Side by side along Y for the sheet; the views aim at the whole row, then at street level."""
    global REVIEW_VIEWS
    gap = 3.0
    widths = []
    for o in objs:
        lo, hi = core.bounds([o])
        widths.append((lo.y, hi.y))
    total = sum(b - a for a, b in widths) + gap * (len(objs) - 1)
    y = total / 2
    for o, (a, b) in zip(objs, widths):
        o.location.y = y - b
        y -= (b - a) + gap
    bpy.context.view_layer.update()
    lo, hi = core.bounds(objs)
    size = (hi - lo).length
    cy = (lo.y + hi.y) / 2
    first = objs[0]
    fl, fh = core.bounds([first])
    REVIEW_VIEWS = [('front', -90, 3, max(total, hi.z) * 2.6 / size, (-1.0, cy, hi.z / 2)),
                    ('angle', -58, 8, max(total, hi.z) * 2.4 / size, (-1.0, cy, hi.z / 2)),
                    ('street', -70, 4, 7.0 / size, (-0.5, (fl.y + fh.y) / 2 + 1.5, 1.8))]
