"""SM_Tenement: the building across the street from the apartment, seen through the rain from the desk.

A six-storey brick walk-up over a row of shops:
- Wet red brick in running bond, soot and rain streaks running down from the sills, a limestone belt course over
  the shops, a pressed cornice and a brick parapet.
- Double-hung windows in deep reveals: cream-painted sashes, stone sills and lintels. About a third are lit (warm
  lamps, the blue of a television, half-drawn blinds), each one different; the rest are dark glass. A few
  have air conditioners hanging off the sill.
- An iron fire escape down two bays: platforms with railings at every floor, stairs between them, a drop ladder.
- The laundromat at street level: a long shop window lit cool white, the dryers' dark fronts inside, a green
  fascia over it, and the tenants' door with a lit transom.

Coordinates (meters): origin at street level on the facade's face, Z up, the facade facing -X (toward the
apartment), Y along the street. The stage places it 18 m from the apartment window, the street 12 m below.
"""
import math
import random

import bmesh
import bpy  # noqa: F401
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import noise, object_coords, ramp, scaled, vec

MESHES = ['SM_Tenement']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'tenement_brick': 4096, 'tenement_stone': 2048, 'tenement_sash': 1024, 'tenement_glass': 256, 'tenement_iron': 1024,
                 'tenement_ac': 512, 'tenement_room_warm': 2048, 'tenement_room_tv': 1024, 'tenement_room_blinds': 2048, 'tenement_store': 2048,
                 'tenement_fascia': 512, 'tenement_door': 512}
AO_DISTANCE = 0.6
REVIEW_VIEWS = [('front', -90, 4, 34.0, (0.0, 0.0, 12.5)), ('angle', -60, 12, 30.0, (0.0, 0.0, 12.5)), ('detail', -75, 6, 9.0, (0.0, -8.0, 12.0))]
REVIEW_SCREEN_LIGHT = False

W = 36.0            # facade width (Y)
DEPTH = 0.45        # the wall's thickness: the windows' reveals
SHOP_H = 4.4        # the shops' storey
FLOOR_H = 3.2
FLOORS = 6
BAY = 2.75
BAYS = 13
WIN_W, WIN_H, SILL = 1.15, 1.75, 0.85   # a window: width, height, its sill above the floor
SASH_X = 0.2        # how deep the sashes sit in the reveal
TOP = SHOP_H + FLOORS * FLOOR_H          # the cornice's underside
PARAPET = TOP + 1.25
# The fire escape runs down in front of these: left of the apartment window's middle (Blender +Y is the view's
# left once the building faces the window), so it balances the neon on the right.
ESCAPE_BAYS = (10, 11)
SHOP_Y = (-8.6, 8.6)                     # the laundromat's window, along the street
DOOR_BAY = 11                            # the tenants' door
# The laundromat's neon (the game's WASH & FOLD sign, 6.4 x 2 m) is fixed to the brick over these windows' places
# on the third floor: bricked up behind it.
SIGN_BLANK = {(6, 3), (7, 3), (8, 3)}
RND = random.Random(1907)


def bay_y(i):
    return (i - (BAYS - 1) / 2) * BAY


def floor_z(k):
    """The floor of storey k (1 = the first floor over the shops)."""
    return SHOP_H + (k - 1) * FLOOR_H


# The facade's own frame: (u along the street, v up, w into the wall) -> Blender (x, y, z), facing -X.
FACE = Matrix(((0.0, 0.0, 1.0, 0.0), (1.0, 0.0, 0.0, 0.0), (0.0, 1.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0)))


def box(name, y0, y1, z0, z1, x0, x1, mat):
    """An axis-aligned box in Blender coordinates (x is depth: negative stands out from the wall)."""
    o = parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)))
    core.assign(o, mat)
    return o


# ------------------------------------------------------------------ materials

def brick_material():
    m = core.Mat('tenement_brick')
    tc, sep = object_coords(m)
    y, z = sep.outputs['Y'], sep.outputs['Z']
    face = vec(m, y, z, sep.outputs['X'])
    # Running bond: 21.5 x 6.5 cm bricks, 1 cm joints.
    br = m.node('ShaderNodeTexBrick', Scale=1.0, **{'Mortar Size': 0.0105, 'Mortar Smooth': 0.15, 'Bias': 0.0, 'Brick Width': 0.225, 'Row Height': 0.075})
    br.offset = 0.5
    br.offset_frequency = 2
    br.squash = 1.0
    br.inputs['Color1'].default_value = core.hex_linear(0x7d3a2a)
    br.inputs['Color2'].default_value = core.hex_linear(0x5b281e)
    br.inputs['Mortar'].default_value = core.hex_linear(0x7d776c)
    m.link(face, br.inputs['Vector'])
    mortar = br.outputs['Fac']
    # Brick to brick: some darker clinkers, some washed out; the whole wall mottled by weather.
    speck = noise(m, scaled(m, sep, 1.0, 9.0, 26.0), 1.0, 2.0)
    mottle = noise(m, tc.outputs['Object'], 0.35, 3.0)
    base = m.mix(m.math('MULTIPLY', ramp(m, speck, 0.55, 0.75), 0.5), br.outputs['Color'], core.hex_linear(0x3b1a14))
    base = m.mix(m.math('MULTIPLY', ramp(m, mottle, 0.45, 0.75), 0.35), base, core.hex_linear(0x8b5a48))
    # Soot and rain: dark streaks running down the wall (stretched noise), heavier toward the top and the street.
    streak = noise(m, scaled(m, sep, 1.0, 3.2, 0.12), 1.0, 3.0)
    soot = m.math('MULTIPLY', ramp(m, streak, 0.5, 0.72), 0.55)
    base = m.mix(soot, base, core.hex_linear(0x1a0f0c))
    grime = ramp(m, z, 2.5, 0.0)
    base = m.mix(m.math('MULTIPLY', grime, 0.6), base, core.hex_linear(0x231812))
    # Wet: darker and glossier where the streaks run; the joints stay matte.
    wet = m.math('MAXIMUM', ramp(m, streak, 0.45, 0.7), m.math('MULTIPLY', ramp(m, mottle, 0.4, 0.8), 0.6))
    base = m.mix(m.math('MULTIPLY', wet, 0.35), base, m.mix(0.5, base, core.hex_linear(0x000000)))
    m.set('Base Color', base)
    rough_brick = m.math('SUBTRACT', 0.62, m.math('MULTIPLY', wet, 0.32))
    # The joints are matte (0.85), the brick faces as rough as they are wet.
    m.set('Roughness', m.math('ADD', m.math('MULTIPLY', mortar, m.math('SUBTRACT', 0.85, rough_brick)), rough_brick))
    # The joints sit back from the brick faces; the faces are a little rough.
    bump = m.node('ShaderNodeBump', Strength=0.55, Distance=0.004)
    m.link(m.math('ADD', m.math('MULTIPLY', m.math('SUBTRACT', 1.0, mortar), 1.0), m.math('MULTIPLY', speck, 0.15)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def stone_material(name='tenement_stone', hex_color=0xb2a68c):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    dirt = noise(m, scaled(m, sep, 1.0, 2.0, 0.4), 1.0, 3.0)
    pores = noise(m, o, 60.0, 2.0)
    color = m.mix(m.math('MULTIPLY', ramp(m, dirt, 0.4, 0.75), 0.75), core.hex_linear(hex_color), core.hex_linear(0x3f382c))
    color = m.mix(m.math('MULTIPLY', pores, 0.15), color, core.hex_linear(0x7c7466))
    m.set('Base Color', color)
    m.set('Roughness', m.math('SUBTRACT', 0.68, m.math('MULTIPLY', ramp(m, dirt, 0.5, 0.8), 0.25)))
    bump = m.node('ShaderNodeBump', Strength=0.3, Distance=0.002)
    m.link(pores, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def paint_material(name, hex_color, rough=0.5, chips_hex=0x5a5248):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    peel = ramp(m, noise(m, o, 9.0, 4.0), 0.62, 0.7)
    dirt = noise(m, scaled(m, sep, 1.0, 1.0, 0.3), 2.0, 3.0)
    color = m.mix(peel, core.hex_linear(hex_color), core.hex_linear(chips_hex))
    color = m.mix(m.math('MULTIPLY', ramp(m, dirt, 0.45, 0.8), 0.5), color, core.hex_linear(0x2a2620))
    m.set('Base Color', color)
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', peel, 0.2)))
    return m


def glass_material():
    m = core.Mat('tenement_glass')
    tc, _ = object_coords(m)
    smear = noise(m, tc.outputs['Object'], 3.0, 3.0)
    m.set('Base Color', m.mix(m.math('MULTIPLY', smear, 0.5), core.hex_linear(0x07090c), core.hex_linear(0x101318)))
    m.set('Roughness', m.math('ADD', 0.06, m.math('MULTIPLY', smear, 0.12)))
    m.set('Specular IOR Level', 0.5)
    return m


def iron_material():
    m = core.Mat('tenement_iron')
    tc, _ = object_coords(m)
    o = tc.outputs['Object']
    rust = ramp(m, noise(m, o, 4.0, 4.0), 0.6, 0.75)
    m.set('Base Color', m.mix(rust, core.hex_linear(0x141518), core.hex_linear(0x4a2414)))
    m.set('Roughness', m.math('ADD', 0.45, m.math('MULTIPLY', rust, 0.4)))
    m.set('Metallic', m.math('SUBTRACT', 0.55, m.math('MULTIPLY', rust, 0.5)))
    return m


def cell_coords(m, sep):
    """Where in its window a point is (u across, v up, 0..1) and a random value for the window it's in."""
    y, z = sep.outputs['Y'], sep.outputs['Z']
    by = m.math('ADD', m.math('DIVIDE', y, BAY), 0.5 + (BAYS - 1) / 2)
    bz = m.math('DIVIDE', m.math('SUBTRACT', z, SHOP_H), FLOOR_H)
    cell = vec(m, m.math('FLOOR', by), m.math('FLOOR', bz), 0.0)
    wn = m.node('ShaderNodeTexWhiteNoise')
    wn.noise_dimensions = '3D'
    m.link(cell, wn.inputs['Vector'])
    # u, v across the window opening itself.
    u = m.math('DIVIDE', m.math('SUBTRACT', m.math('MULTIPLY', m.math('FRACT', by), BAY), (BAY - WIN_W) / 2), WIN_W)
    v = m.math('DIVIDE', m.math('SUBTRACT', m.math('MULTIPLY', m.math('FRACT', bz), FLOOR_H), SILL), WIN_H)
    return u, v, wn.outputs['Value'], wn.outputs['Color']


def facing_viewer(m):
    """1 on the back of a room (facing the street), less on its walls, floor and ceiling."""
    geo = m.node('ShaderNodeNewGeometry')
    sep = m.node('ShaderNodeSeparateXYZ')
    m.link(geo.outputs['Normal'], sep.inputs['Vector'])
    return m.math('ADD', 0.35, m.math('MULTIPLY', m.math('ABSOLUTE', sep.outputs['X']), 0.65))


def room_material(kind):
    """A lit room behind a window (emissive), different in every window: the lamp, the curtains, the blinds."""
    m = core.Mat(f'tenement_room_{kind}')
    tc, sep = object_coords(m)
    u, v, rnd, rcol = cell_coords(m, sep)
    walls = facing_viewer(m)
    if kind == 'warm':
        # A lamp somewhere in the room: a pool of warm light on the back wall, the corners falling off.
        lx = m.math('ADD', 0.2, m.math('MULTIPLY', rnd, 0.6))
        dx = m.math('SUBTRACT', u, lx)
        dy = m.math('SUBTRACT', v, 0.42)
        d = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', dx, dx), m.math('MULTIPLY', m.math('MULTIPLY', dy, dy), 0.6)))
        pool = ramp(m, d, 0.75, 0.05)
        tint = m.mix(m.math('MULTIPLY', ramp(m, rnd, 0.3, 0.9), 1.0), core.hex_linear(0xffb468), core.hex_linear(0xffd9a0))
        lit = m.mix(pool, core.hex_linear(0x3a1d0a), tint)
        # Curtains drawn part way in from the sides (how far: the window's own random).
        cut = m.math('MULTIPLY', ramp(m, rnd, 0.0, 1.0), 0.3)
        curtain = m.math('MAXIMUM', ramp(m, m.math('SUBTRACT', cut, u), -0.02, 0.02),
                         ramp(m, m.math('SUBTRACT', u, m.math('SUBTRACT', 1.0, cut)), -0.02, 0.02))
        lit = m.mix(m.math('MULTIPLY', curtain, 0.8), lit, m.mix(0.5, lit, core.hex_linear(0x1a0c06)))
        strength = 5.0
    elif kind == 'tv':
        # A dark room lit by a television low down: cold blue, brightest near the floor, a little colour in it.
        glow = m.math('MULTIPLY', ramp(m, v, 0.95, -0.1), ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', u, m.math('ADD', 0.3, m.math('MULTIPLY', rnd, 0.4)))), 0.7, 0.0))
        tint = m.mix(m.math('MULTIPLY', rnd, 0.5), core.hex_linear(0x4f7de0), m.mix(0.5, rcol, core.hex_linear(0x6f8fff)))
        lit = m.mix(glow, core.hex_linear(0x05070f), tint)
        strength = 3.0
    else:
        # Venetian blinds, lowered to a different height in each window, lamplight between the slats.
        level = m.math('ADD', 0.15, m.math('MULTIPLY', rnd, 0.75))
        slats = ramp(m, m.math('SINE', m.math('MULTIPLY', sep.outputs['Z'], 2.0 * math.pi / 0.032)), -0.2, 0.6)
        below = ramp(m, m.math('SUBTRACT', level, v), -0.01, 0.01)
        warm = m.mix(ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', u, 0.5)), 0.7, 0.0), core.hex_linear(0x5a3414), core.hex_linear(0xffcf8a))
        blind = m.mix(slats, m.mix(0.75, warm, core.hex_linear(0x000000)), m.mix(0.15, warm, core.hex_linear(0xffffff)))
        lit = m.mix(below, blind, warm)
        strength = 4.0
    lit = m.mix(m.math('SUBTRACT', 1.0, walls), lit, m.mix(0.7, lit, core.hex_linear(0x000000)))
    m.set('Base Color', core.hex_linear(0x0d0c0b))
    m.set('Emission Color', lit)
    m.set('Emission Strength', strength)
    m.set('Roughness', 0.2)
    return m


def store_material():
    """The laundromat after dark: fluorescent white, the dryers' dark fronts and their round doors."""
    m = core.Mat('tenement_store')
    tc, sep = object_coords(m)
    y, z = sep.outputs['Y'], sep.outputs['Z']
    walls = facing_viewer(m)
    col = m.math('FRACT', m.math('DIVIDE', y, 0.82))
    cabinet = m.math('MULTIPLY', ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', col, 0.5)), 0.46, 0.4), ramp(m, z, 2.25, 2.15))
    row_z = m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', m.math('SUBTRACT', z, 0.25), 0.98)), 0.5)
    door_d = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', m.math('SUBTRACT', col, 0.5), m.math('SUBTRACT', col, 0.5)), m.math('MULTIPLY', row_z, row_z)))
    door = m.math('MULTIPLY', ramp(m, door_d, 0.3, 0.26), cabinet)
    ceiling = ramp(m, z, 2.6, 3.4)
    lit = m.mix(cabinet, core.hex_linear(0xdfeaff), core.hex_linear(0x3b3e44))
    lit = m.mix(door, lit, core.hex_linear(0x0f1216))
    lit = m.mix(m.math('MULTIPLY', ceiling, 0.5), lit, core.hex_linear(0xffffff))
    lit = m.mix(m.math('SUBTRACT', 1.0, walls), lit, m.mix(0.6, lit, core.hex_linear(0x000000)))
    m.set('Base Color', core.hex_linear(0x101112))
    m.set('Emission Color', lit)
    m.set('Emission Strength', 6.0)
    m.set('Roughness', 0.15)
    return m


def materials():
    return {'brick': brick_material(), 'stone': stone_material(), 'sash': paint_material('tenement_sash', 0xd2c9b2, 0.45),
            'glass': glass_material(), 'iron': iron_material(), 'ac': paint_material('tenement_ac', 0xbdb6a6, 0.4, chips_hex=0x6b5340),
            'warm': room_material('warm'), 'tv': room_material('tv'), 'blinds': room_material('blinds'), 'store': store_material(),
            'fascia': paint_material('tenement_fascia', 0x163b2c, 0.35, chips_hex=0x0d1f18), 'door': paint_material('tenement_door', 0x2a1a12, 0.5)}


# ------------------------------------------------------------------ geometry

def windows():
    """Every window: (bay, storey, how it's lit: None, 'warm', 'tv' or 'blinds', with an air conditioner)."""
    out = []
    for k in range(1, FLOORS + 1):
        for i in range(BAYS):
            r = RND.random()
            if (i, k) in SIGN_BLANK:
                continue
            lit = None if r < 0.62 else ('warm' if r < 0.8 else 'blinds' if r < 0.92 else 'tv')
            ac = lit is None and i not in ESCAPE_BAYS and RND.random() < 0.16
            out.append((i, k, lit, ac))
    return out


def wall(mats, wins):
    """The brick face with its window openings, the shop window and the door cut through it."""
    holes = []
    for i, k, _, _ in wins:
        y, z = bay_y(i), floor_z(k) + SILL
        holes.append([(y - WIN_W / 2, z), (y - WIN_W / 2, z + WIN_H), (y + WIN_W / 2, z + WIN_H), (y + WIN_W / 2, z)])
    holes.append([(SHOP_Y[0], 0.45), (SHOP_Y[0], 3.35), (SHOP_Y[1], 3.35), (SHOP_Y[1], 0.45)])
    dy = bay_y(DOOR_BAY)
    holes.append([(dy - 0.65, 0.0), (dy - 0.65, 3.2), (dy + 0.65, 3.2), (dy + 0.65, 0.0)])
    outer = [(-W / 2, 0.0), (W / 2, 0.0), (W / 2, PARAPET), (-W / 2, PARAPET)]
    o = parts.outline_prism('brick', outer, holes, 0.0, DEPTH, FACE)
    # Nobody sees the back of the wall (the rooms and the shop are boxes of their own): it goes, so the brick's
    # texture is all front and reveals.
    bm = bmesh.new()
    bm.from_mesh(o.data)
    bm.normal_update()
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.normal.x > 0.9 and f.calc_center_median().x > DEPTH - 0.01], context='FACES')
    bm.to_mesh(o.data)
    bm.free()
    core.assign(o, mats['brick'])
    return [o]


def window_unit(i, k, lit, ac, mats):
    """One window: the sashes, the glass (or the room behind), the sill and the lintel, maybe an air conditioner."""
    y, z0 = bay_y(i), floor_z(k) + SILL
    y0, y1, z1 = y - WIN_W / 2, y + WIN_W / 2, z0 + WIN_H
    out = []
    f, x0, x1 = 0.06, SASH_X, SASH_X + 0.07
    sash = mats['sash']
    out += [box('frame', y0, y0 + f, z0, z1, x0, x1, sash), box('frame', y1 - f, y1, z0, z1, x0, x1, sash),
            box('frame', y0, y1, z1 - f, z1, x0, x1, sash), box('frame', y0, y1, z0, z0 + f * 1.4, x0, x1, sash)]
    zm = (z0 + z1) / 2
    out.append(box('rail', y0, y1, zm - 0.03, zm + 0.03, x0 - 0.03, x1 - 0.03, sash))
    out.append(box('muntin', y - 0.018, y + 0.018, zm, z1, x0 - 0.01, x1 - 0.01, sash))
    if lit:
        # The room behind: a box open toward the street, deep enough that the reveal hides its edges.
        rm = mats[lit]
        bx0, bx1 = DEPTH, DEPTH + 1.6
        out += [box('room', y0 - 0.5, y1 + 0.5, z0 - 0.9, z1 + 0.6, bx1, bx1 + 0.02, rm),
                box('room', y0 - 0.5, y0 - 0.48, z0 - 0.9, z1 + 0.6, bx0, bx1, rm), box('room', y1 + 0.48, y1 + 0.5, z0 - 0.9, z1 + 0.6, bx0, bx1, rm),
                box('room', y0 - 0.5, y1 + 0.5, z1 + 0.58, z1 + 0.6, bx0, bx1, rm), box('room', y0 - 0.5, y1 + 0.5, z0 - 0.9, z0 - 0.88, bx0, bx1, rm)]
    else:
        out.append(box('glass', y0 + f, y1 - f, z0 + f, z1 - f, x0 + 0.03, x0 + 0.035, mats['glass']))
    stone = mats['stone']
    out.append(box('sill', y0 - 0.1, y1 + 0.1, z0 - 0.09, z0, -0.07, SASH_X, stone))
    out.append(box('lintel', y0 - 0.14, y1 + 0.14, z1, z1 + 0.24, -0.025, 0.02, stone))
    out.append(box('keystone', y - 0.11, y + 0.11, z1 - 0.02, z1 + 0.28, -0.045, 0.02, stone))
    if ac:
        out.append(box('ac', y - 0.34, y + 0.34, z0, z0 + 0.42, -0.38, SASH_X - 0.02, mats['ac']))
        out.append(box('ac_brace', y - 0.3, y - 0.27, z0 - 0.35, z0, -0.32, 0.0, mats['iron']))
        out.append(box('ac_brace', y + 0.27, y + 0.3, z0 - 0.35, z0, -0.32, 0.0, mats['iron']))
    return out


def trim(mats):
    """The belt course over the shops, the cornice and its brackets, the coping on the parapet."""
    s = mats['stone']
    out = [box('belt', -W / 2, W / 2, SHOP_H - 0.12, SHOP_H + 0.22, -0.12, 0.05, s),
           box('belt', -W / 2, W / 2, SHOP_H + 0.22, SHOP_H + 0.3, -0.06, 0.05, s)]
    # The cornice: three steps out from the wall, brackets under the top one.
    out += [box('cornice', -W / 2, W / 2, TOP, TOP + 0.18, -0.12, 0.05, s), box('cornice', -W / 2, W / 2, TOP + 0.18, TOP + 0.34, -0.3, 0.05, s),
            box('cornice', -W / 2, W / 2, TOP + 0.62, TOP + 0.78, -0.62, 0.05, s), box('cornice', -W / 2, W / 2, TOP + 0.34, TOP + 0.62, -0.36, 0.05, s)]
    n = int(W / 0.9)
    for j in range(n + 1):
        y = -W / 2 + 0.3 + j * 0.9
        if y > W / 2 - 0.3:
            break
        out.append(box('bracket', y - 0.08, y + 0.08, TOP + 0.34, TOP + 0.62, -0.58, -0.3, s))
    out.append(box('coping', -W / 2, W / 2, PARAPET, PARAPET + 0.1, -0.08, DEPTH, s))
    return out


def shopfront(mats):
    """The laundromat: its window and the room behind it, mullions, the fascia; the tenants' door."""
    out = []
    y0, y1 = SHOP_Y
    # The shop inside: fluorescent white, the dryers along the back wall.
    st = mats['store']
    out += [box('shop', y0 - 0.5, y1 + 0.5, 0.0, 3.6, DEPTH + 3.4, DEPTH + 3.42, st),
            box('shop', y0 - 0.5, y1 + 0.5, 3.55, 3.6, DEPTH, DEPTH + 3.4, st), box('shop', y0 - 0.5, y1 + 0.5, 0.0, 0.05, DEPTH, DEPTH + 3.4, st),
            box('shop', y0 - 0.52, y0 - 0.5, 0.0, 3.6, DEPTH, DEPTH + 3.4, st), box('shop', y1 + 0.5, y1 + 0.52, 0.0, 3.6, DEPTH, DEPTH + 3.4, st)]
    # Mullions every 2.15 m, a transom bar, the stall riser under the glass.
    fr = mats['fascia']
    n = 8
    for j in range(n + 1):
        y = y0 + (y1 - y0) * j / n
        out.append(box('mullion', y - 0.05, y + 0.05, 0.45, 3.35, 0.12, 0.22, fr))
    out.append(box('transom', y0, y1, 2.7, 2.78, 0.12, 0.22, fr))
    out.append(box('riser', y0, y1, 0.0, 0.45, 0.05, 0.25, mats['stone']))
    # The fascia over it, deep green, with a thin gold line.
    out.append(box('fascia', y0 - 0.4, y1 + 0.4, 3.35, 4.2, -0.18, 0.05, fr))
    # The tenants' door: dark wood in the opening, a lit transom over it, a lamp beside it.
    dy = bay_y(DOOR_BAY)
    out.append(box('door', dy - 0.55, dy + 0.55, 0.0, 2.45, 0.28, 0.33, mats['door']))
    out.append(box('transom_light', dy - 0.55, dy + 0.55, 2.55, 3.1, 0.35, 0.37, mats['warm']))
    out.append(box('door_head', dy - 0.8, dy + 0.8, 3.2, 3.45, -0.06, 0.05, mats['stone']))
    return out


def fire_escape(mats):
    """Platforms at each floor across two bays, railings, stairs down between them, the drop ladder."""
    iron = mats['iron']
    out = []
    ya = bay_y(ESCAPE_BAYS[0]) - BAY / 2 + 0.15
    yb = bay_y(ESCAPE_BAYS[1]) + BAY / 2 - 0.15
    deep = 1.15
    for k in range(1, FLOORS + 1):
        z = floor_z(k) + SILL - 0.18
        # The deck (a grating, dark from below), its frame, and brackets back to the wall.
        out.append(box('deck', ya, yb, z - 0.03, z, -deep, -0.02, iron))
        out.append(box('deck_edge', ya, yb, z - 0.1, z, -deep, -deep + 0.04, iron))
        for y in (ya + 0.1, (ya + yb) / 2, yb - 0.1):
            out.append(parts.rod('strut', (-deep + 0.1, y, z - 0.03), (-0.02, y, z - 0.75), 0.018, 8))
        # Railings: a top rail and a mid rail, balusters every 12 cm, on the front and both ends.
        rail_z = z + 0.95
        for zz in (rail_z, z + 0.5):
            out.append(parts.rod('rail', (-deep, ya, zz), (-deep, yb, zz), 0.016, 8))
            out.append(parts.rod('rail', (-deep, ya, zz), (-0.02, ya, zz), 0.016, 8))
            out.append(parts.rod('rail', (-deep, yb, zz), (-0.02, yb, zz), 0.016, 8))
        n = int((yb - ya) / 0.12)
        for j in range(n + 1):
            y = ya + (yb - ya) * j / n
            out.append(parts.rod('baluster', (-deep + 0.02, y, z), (-deep + 0.02, y, rail_z), 0.007, 6))
        for x in [-deep + 0.02 + (deep - 0.04) * j / 8 for j in range(9)]:
            out.append(parts.rod('baluster', (x, ya, z), (x, ya, rail_z), 0.007, 6))
            out.append(parts.rod('baluster', (x, yb, z), (x, yb, rail_z), 0.007, 6))
        # The stairs up to the next platform, along the front, from one end to the other (alternating).
        if k < FLOORS:
            z2 = floor_z(k + 1) + SILL - 0.18
            s0, s1 = (ya + 0.35, yb - 1.2) if k % 2 else (yb - 0.35, ya + 1.2)
            xs = -deep + 0.35
            for side in (-0.28, 0.28):
                out.append(parts.rod('stringer', (xs + side, s0, z), (xs + side, s1, z2), 0.02, 8))
            steps = 11
            for j in range(1, steps):
                t = j / steps
                yy, zz = s0 + (s1 - s0) * t, z + (z2 - z) * t
                out.append(box('tread', yy - 0.09, yy + 0.09, zz - 0.02, zz, xs - 0.3, xs + 0.3, iron))
            out.append(parts.rod('handrail', (xs - 0.3, s0, z + 0.85), (xs - 0.3, s1, z2 + 0.85), 0.014, 8))
    # The drop ladder under the lowest platform, pulled up.
    z1 = floor_z(1) + SILL - 0.18
    ly = yb - 0.6
    for side in (-0.22, 0.22):
        out.append(parts.rod('ladder', (-deep + 0.15, ly + side, z1 - 0.1), (-deep + 0.15, ly + side, z1 - 2.6), 0.016, 8))
    for j in range(9):
        zz = z1 - 0.3 - j * 0.28
        out.append(parts.rod('rung', (-deep + 0.15, ly - 0.22, zz), (-deep + 0.15, ly + 0.22, zz), 0.011, 6))
    for o in out:
        if not o.data.materials:
            core.assign(o, iron)
    return out


def build():
    core.reset()
    mats = materials()
    wins = windows()
    objs = wall(mats, wins)
    for i, k, lit, ac in wins:
        objs += window_unit(i, k, lit, ac, mats)
    objs += trim(mats)
    objs += shopfront(mats)
    objs += fire_escape(mats)
    obj = core.join(objs, 'SM_Tenement')
    obj.name = obj.data.name = 'SM_Tenement'
    core.finish_hard_surface(obj, 30.0)
    # The brick's face is mapped flat across most of its texture (about 9 mm a texel, a pixel or so from the
    # apartment); its reveals pack into a strip along the top. Every other material gets a texture of its own.
    brick = core.material_is(obj, 'tenement_brick')
    groups = [(lambda f: brick(f) and f.normal.x < -0.9, 'planar', ('y', 'z', -W / 2, W / 2, 0.0, PARAPET), (0.0, 0.0, 1.0, 0.9)),
              (brick, 'box', None, (0.0, 0.905, 1.0, 1.0))]
    groups += [(core.material_is(obj, s.material.name), 'box', None, (0.0, 0.0, 1.0, 1.0)) for s in obj.material_slots if s.material.name != 'tenement_brick']
    groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
    core.uv_layout(obj, groups)
    return [obj]
