"""The Embercrest's card room kit: a modern tournament room for the Embercrest Poker Series.

The game assembles the room from these pieces (BackRoomStageCardRoom.cpp), instanced, on the old room's plan:
- SM_EC_Wall: a 4 m bay. A black stone skirting; two navy fabric acoustic panels split by a copper reveal; an
  ember LED cove line at 2.8 m (SM_EC_Wall_Glow, with the vertical LED reveals at the bay's edges); smoked oak
  slats above to the black crown.
- SM_EC_Column: black glass between a stone plinth and capital, copper bands, an LED channel at each corner.
- SM_EC_Ceiling: one 4 m coffer: matte black beams, a raised tray with an LED cove round it, a black downlight
  trim at its middle (SM_EC_Ceiling_Glow: the cove and the downlight's lens).
- SM_EC_Pendant: the linear light over every table: a slim black housing with copper edges on two cables
  (SM_EC_Pendant_Glow: the diffuser).
- SM_EC_WallLight: a slim vertical wall light with a glowing slot (SM_EC_WallLight_Glow).
- SM_EC_TableSign: the frame of a table's hanging LED sign (the game shows the faces) on two cables.
- SM_EC_Banner: a printed hanging banner, both sides (the Embercrest's lockup, PLAY / COMPETE / RISE).
- SM_EC_Stage: the final table's stage: a gloss black deck, a fascia with a copper nosing, steps at both ends,
  LED pylons each side and a truss across (SM_EC_Stage_Glow: the ember edge lines and the pylons). The game puts
  the backdrop screen behind it.

Coordinates (meters) as the old kit: every piece stands on the floor (Z up) with its face toward -X, its width
along Y, centered on its origin. The ceiling coffer hangs from z = 0 upward; the pendant hangs from z = 2.0 down
to its diffuser at 0; the banner and the sign hang from z = 0 down.
"""
import math
import os
import sys

import bmesh
import bpy  # noqa: F401
from mathutils import Matrix

from artkit import core, parts
from artkit.shade import noise, object_coords, ramp, scaled, vec

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import brand  # noqa: E402
from artkit.sheet import Sheet  # noqa: E402

MESHES = ['SM_EC_Wall', 'SM_EC_Wall_Glow', 'SM_EC_Column', 'SM_EC_Column_Glow', 'SM_EC_Ceiling', 'SM_EC_Ceiling_Glow', 'SM_EC_Pendant',
          'SM_EC_Pendant_Glow', 'SM_EC_WallLight', 'SM_EC_WallLight_Glow', 'SM_EC_TableSign', 'SM_EC_Banner', 'SM_EC_Stage', 'SM_EC_Stage_Glow']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'ec_fabric': 2048, 'ec_oak': 2048, 'ec_banner': 2048, 'ec_stage_deck': 2048}
AO_DISTANCE = 0.2
REVIEW_VIEWS = [('front', -90, 8, 9.0, (0.0, 0.0, 1.8)), ('angle', -55, 18, 9.0, (0.0, 0.0, 1.8)), ('detail', -70, 10, 3.0, (0.0, 0.0, 1.2))]
REVIEW_SCREEN_LIGHT = False

WALL_W = 4.0
WALL_H = 4.2
COVE_Z = 2.8
BANNER_W, BANNER_H = 1.3, 3.4
BANNER_TOP = -0.35


# ------------------------------------------------------------------ materials

def oak_material(name='ec_oak'):
    """Smoked oak: dark, cool brown, a long grain along Z with open pores, satin."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    grain = noise(m, scaled(m, sep, 60.0, 60.0, 2.2), detail=6.0, distortion=1.5)
    pores = ramp(m, noise(m, scaled(m, sep, 900.0, 900.0, 40.0), detail=2.0), 0.62, 0.7)
    col = m.mix(grain, core.hex_linear(0x1c1612), core.hex_linear(0x3a2c22))
    m.set('Base Color', m.mix(m.math('MULTIPLY', pores, 0.5), col, core.hex_linear(0x0c0a08)))
    m.set('Roughness', m.math('ADD', 0.42, m.math('MULTIPLY', pores, 0.25)))
    bump = m.node('ShaderNodeBump', Strength=0.25, Distance=0.0006)
    m.link(m.math('SUBTRACT', grain, m.math('MULTIPLY', pores, 0.6)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def stone_material(name='ec_stone', hex_color=0x0d0e11, rough=0.28):
    """Black stone: a few fine pale veins in a polished field."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    warp = noise(m, tc.outputs['Object'], scale=3.0, detail=4.0, distortion=3.0)
    v = noise(m, vec(m, m.math('ADD', sep.outputs['X'], m.math('MULTIPLY', warp, 0.6)), sep.outputs['Y'], m.math('ADD', sep.outputs['Z'], warp)), scale=9.0, detail=8.0)
    vein = ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', v, 0.5)), 0.012, 0.0)
    m.set('Base Color', m.mix(m.math('MULTIPLY', vein, 0.55), core.hex_linear(hex_color), core.hex_linear(0x8a8c90)))
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', vein, 0.1)))
    return m


def banner_material():
    """The banner's print, laid on by position: u across the banner (Y), v up it (Z), mirrored per face so both
    sides read (Y runs the other way in the game, the export mirrors it: the front takes the flipped u)."""
    s = Sheet(BANNER_W * 1000.0, BANNER_H * 1000.0)
    brand.banner_art(s, BANNER_W * 1000.0, BANNER_H * 1000.0)
    color_path, _surface = s.render('ec_banner_print', 1024, samples=24)
    m = core.Mat('ec_banner')
    tc, sep = object_coords(m)
    geo = m.node('ShaderNodeNewGeometry')
    nsep = m.node('ShaderNodeSeparateXYZ')
    m.link(geo.outputs['Normal'], nsep.inputs['Vector'])
    u = m.math('ADD', m.math('DIVIDE', sep.outputs['Y'], BANNER_W), 0.5)
    back = m.math('GREATER_THAN', nsep.outputs['X'], 0.0)
    u = m.mix(back, m.math('SUBTRACT', 1.0, u), u)
    v = m.math('DIVIDE', m.math('SUBTRACT', sep.outputs['Z'], BANNER_TOP - BANNER_H), BANNER_H)
    col = m.image(color_path, vector=vec(m, u, v, 0.0), extension='EXTEND')
    heather = noise(m, tc.outputs['Object'], scale=700.0, detail=2.0)
    m.set('Base Color', m.mix(m.math('MULTIPLY', heather, 0.08), col.outputs['Color'], core.hex_linear(0x000000)))
    m.set('Roughness', 0.82)
    m.set('Sheen Weight', 0.4)
    return m


def mats():
    return {
        'fabric': parts.fabric('ec_fabric', 0x1d2947, period=0.0022, rough=0.9, sheen=0.35, hex_alt=0x25325a),
        'stone': stone_material(),
        'copper': parts.metal('ec_copper', 0xb8743f, 0.26, brushed_axis='Z'),
        'oak': oak_material(),
        'black': parts.metal('ec_black', 0x121316, 0.5, anodized=True),
        'blackglass': parts.gloss('ec_blackglass', 0x06070a, 0.05, coat=0.3),
        'ceiling': parts.plastic('ec_ceiling', 0x1b1d23, 0.9, scuffs=0.0),
        'deck': stone_material('ec_stage_deck', 0x08090c, 0.12),
        'banner': banner_material(),
    }


# ------------------------------------------------------------------ shapes

def box(name, x0, x1, y0, y1, z0, z1, mat, r=0.0):
    o = parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), r, 2)
    if mat is not None:
        core.assign(o, mat)
    return o


def lit_box(name, x0, x1, y0, y1, z0, z1):
    return parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), 0.0, 1)


# ------------------------------------------------------------------ the pieces

def wall(M):
    out, lit = [], []
    hw = WALL_W / 2
    out.append(box('wall', 0.0, 0.12, -hw, hw, 0.0, WALL_H, M['fabric']))
    # The skirting, the two fabric panels and the copper reveal between them.
    out.append(box('skirt', -0.03, 0.0, -hw, hw, 0.0, 0.18, M['stone'], 0.004))
    for c in (-hw / 2, hw / 2):
        out.append(box('panel', -0.05, 0.0, c - hw / 2 + 0.03, c + hw / 2 - 0.03, 0.22, COVE_Z - 0.1, M['fabric'], 0.012))
    out.append(box('reveal', -0.035, 0.0, -0.014, 0.014, 0.22, COVE_Z - 0.1, M['copper'], 0.003))
    # The cove: a stone ledge with the LED line set into its top, light thrown up the slats.
    out.append(box('ledge', -0.11, 0.0, -hw, hw, COVE_Z - 0.1, COVE_Z - 0.02, M['stone'], 0.006))
    out.append(box('lip', -0.115, -0.09, -hw, hw, COVE_Z - 0.02, COVE_Z + 0.04, M['stone'], 0.004))
    lit.append(lit_box('cove', -0.088, -0.03, -hw, hw, COVE_Z - 0.02, COVE_Z - 0.008))
    # The vertical LED reveals at the bay's edges (a pair meets at every bay line).
    for y in (-hw + 0.004, hw - 0.004):
        lit.append(lit_box('edge', -0.012, 0.0, y - 0.004, y + 0.004, 0.2, COVE_Z - 0.1))
    # Smoked oak slats over a black backing, up to the crown.
    out.append(box('backing', -0.01, 0.0, -hw, hw, COVE_Z + 0.04, WALL_H - 0.14, M['ceiling']))
    n = 40
    for k in range(n):
        y = -hw + (k + 0.5) * WALL_W / n
        out.append(box('slat', -0.045, -0.01, y - 0.022, y + 0.022, COVE_Z + 0.06, WALL_H - 0.16, M['oak'], 0.004))
    out.append(box('crown', -0.08, 0.0, -hw, hw, WALL_H - 0.14, WALL_H, M['black'], 0.008))
    return parts.finish(out, 'SM_EC_Wall', 30.0), parts.glow_object(lit, 'SM_EC_Wall_Glow')


def column(M):
    out, lit = [], []
    s = 0.35
    out.append(box('plinth', -s - 0.03, s + 0.03, -s - 0.03, s + 0.03, 0.0, 0.2, M['stone'], 0.008))
    out.append(box('shaft', -s, s, -s, s, 0.2, WALL_H - 0.36, M['blackglass'], 0.004))
    for z in (1.08, WALL_H - 0.7):
        out.append(box('band', -s - 0.01, s + 0.01, -s - 0.01, s + 0.01, z, z + 0.035, M['copper'], 0.004))
    out.append(box('capital', -s - 0.05, s + 0.05, -s - 0.05, s + 0.05, WALL_H - 0.36, WALL_H, M['stone'], 0.01))
    # An LED channel down each corner.
    for sx in (-1, 1):
        for sy in (-1, 1):
            out.append(box('notch', sx * s - 0.02, sx * s + 0.02, sy * s - 0.02, sy * s + 0.02, 0.2, WALL_H - 0.36, M['black']))
            cx, cy = sx * (s + 0.006), sy * (s + 0.006)
            lit.append(lit_box('led', cx - 0.006, cx + 0.006, cy - 0.006, cy + 0.006, 0.24, WALL_H - 0.4))
    return parts.finish(out, 'SM_EC_Column', 30.0), parts.glow_object(lit, 'SM_EC_Column_Glow')


def ceiling(M):
    out, lit = [], []
    h = 2.0
    beam = 0.12
    depth = 0.26
    tray = 1.3
    for sx in (-1, 1):
        out.append(box('beam', sx * h - (beam if sx > 0 else 0.0), sx * h + (0.0 if sx > 0 else beam), -h, h, 0.0, depth, M['ceiling']))
        out.append(box('beam', -h, h, sx * h - (beam if sx > 0 else 0.0), sx * h + (0.0 if sx > 0 else beam), 0.0, depth, M['ceiling']))
    # The coffer's flat, and the raised tray in its middle with the cove's LED round its rim.
    for (x0, x1, y0, y1) in ((-h + beam, h - beam, -h + beam, -tray), (-h + beam, h - beam, tray, h - beam), (-h + beam, -tray, -tray, tray), (tray, h - beam, -tray, tray)):
        out.append(box('flat', x0, x1, y0, y1, depth, depth + 0.02, M['ceiling']))
    out.append(box('tray', -tray, tray, -tray, tray, depth + 0.18, depth + 0.2, M['ceiling']))
    for sx in (-1, 1):
        out.append(box('tray_side', sx * tray - 0.01, sx * tray + 0.01, -tray, tray, depth + 0.02, depth + 0.18, M['ceiling']))
        out.append(box('tray_side', -tray, tray, sx * tray - 0.01, sx * tray + 0.01, depth + 0.02, depth + 0.18, M['ceiling']))
        lit.append(lit_box('cove', sx * (tray - 0.04) - 0.012, sx * (tray - 0.04) + 0.012, -tray + 0.03, tray - 0.03, depth + 0.172, depth + 0.178))
        lit.append(lit_box('cove', -tray + 0.03, tray - 0.03, sx * (tray - 0.04) - 0.012, sx * (tray - 0.04) + 0.012, depth + 0.172, depth + 0.178))
    trim = parts.lathe_at('trim', [(0.11, depth + 0.18), (0.1, depth + 0.13), (0.08, depth + 0.12), (0.07, depth + 0.16)], Matrix(), 40)
    core.assign(trim, M['black'])
    out.append(trim)
    lit.append(parts.lathe_at('lens', [(0.0, depth + 0.14), (0.075, depth + 0.14)], Matrix(), 32))
    return parts.finish(out, 'SM_EC_Ceiling', 30.0), parts.glow_object(lit, 'SM_EC_Ceiling_Glow')


def pendant(M):
    out, lit = [], []
    top = 2.0
    L, W, H = 1.7, 0.24, 0.07
    out.append(box('housing', -W / 2, W / 2, -L / 2, L / 2, 0.012, H, M['black'], 0.012))
    for sx in (-1, 1):
        out.append(box('edge', sx * W / 2 - 0.004, sx * W / 2 + 0.004, -L / 2 + 0.01, L / 2 - 0.01, H - 0.022, H - 0.012, M['copper']))
    out.append(box('rim', -W / 2 + 0.008, W / 2 - 0.008, -L / 2 + 0.008, L / 2 - 0.008, 0.0, 0.014, M['black'], 0.004))
    lit.append(lit_box('diffuser', -W / 2 + 0.018, W / 2 - 0.018, -L / 2 + 0.02, L / 2 - 0.02, 0.003, 0.006))
    for y in (-0.62, 0.62):
        out.append(parts.rod('cable', (0.0, y, H), (0.0, y, top - 0.03), 0.0015, 8))
        core.assign(out[-1], M['black'])
        can = parts.lathe_at('canopy', [(0.0, top - 0.035), (0.045, top - 0.035), (0.05, top), (0.0, top)], Matrix.Translation((0.0, y, 0.0)), 32)
        core.assign(can, M['black'])
        out.append(can)
    return parts.finish(out, 'SM_EC_Pendant', 35.0), parts.glow_object(lit, 'SM_EC_Pendant_Glow')


def wall_light(M):
    out, lit = [], []
    out.append(box('plate', -0.02, 0.0, -0.045, 0.045, 1.5, 2.5, M['black'], 0.008))
    out.append(box('body', -0.07, -0.02, -0.035, 0.035, 1.52, 2.48, M['black'], 0.01))
    for z in (1.52, 2.46):
        out.append(box('cap', -0.074, -0.018, -0.039, 0.039, z, z + 0.02, M['copper'], 0.003))
    lit.append(lit_box('slot', -0.072, -0.068, -0.012, 0.012, 1.56, 2.44))
    return parts.finish(out, 'SM_EC_WallLight', 35.0), parts.glow_object(lit, 'SM_EC_WallLight_Glow')


def table_sign(M):
    """The sign's frame, hung 0.6 m below its cables' tops: a black box open at both faces (the game puts the faces
    in: 0.84 x 0.42 m at x = +-0.03, centered at z = -0.85)."""
    out = []
    W, H, D = 0.92, 0.5, 0.07
    zc = -0.6 - H / 2
    for sy in (-1, 1):
        y = sy * (W / 2 - 0.02)
        out.append(box('side', -D / 2, D / 2, y - 0.02, y + 0.02, zc - H / 2, zc + H / 2, M['black'], 0.006))
    for sz in (-1, 1):
        z = zc + sz * (H / 2 - 0.02)
        out.append(box('rail', -D / 2, D / 2, -W / 2, W / 2, z - 0.02, z + 0.02, M['black'], 0.006))
    out.append(box('core', -0.025, 0.025, -W / 2 + 0.03, W / 2 - 0.03, zc - H / 2 + 0.03, zc + H / 2 - 0.03, M['ceiling']))
    for sx in (-1, 1):
        out.append(box('trim', sx * D / 2 - 0.004, sx * D / 2 + 0.004, -W / 2, W / 2, zc - H / 2, zc - H / 2 + 0.008, M['copper']))
    for y in (-0.3, 0.3):
        out.append(parts.rod('cable', (0.0, y, zc + H / 2), (0.0, y, 0.0), 0.0015, 8))
        core.assign(out[-1], M['black'])
    return parts.finish(out, 'SM_EC_TableSign', 35.0)


def banner(M):
    """The banner: printed fabric between a top and a bottom rod, a slight sway in it, hung by two cables."""
    out = []
    bm = bmesh.new()
    nx, nz = 24, 48
    z0, z1 = BANNER_TOP - BANNER_H, BANNER_TOP
    for side, xoff in ((-1, -0.0015), (1, 0.0015)):
        verts = []
        for j in range(nz + 1):
            row = []
            z = z0 + (z1 - z0) * j / nz
            for i in range(nx + 1):
                y = -BANNER_W / 2 + BANNER_W * i / nx
                sway = 0.012 * math.sin(i / nx * math.pi * 3.0 + 0.6) * (0.4 + 0.6 * (1.0 - j / nz))
                row.append(bm.verts.new((xoff + sway, y, z)))
            verts.append(row)
        for j in range(nz):
            for i in range(nx):
                q = (verts[j][i], verts[j][i + 1], verts[j + 1][i + 1], verts[j + 1][i])
                bm.faces.new(q if side > 0 else tuple(reversed(q)))
    cloth = core.mesh_object('cloth', bm)
    core.assign(cloth, M['banner'])
    out.append(cloth)
    for z in (z1, z0):
        r = parts.rod('rod', (0.0, -BANNER_W / 2 - 0.03, z), (0.0, BANNER_W / 2 + 0.03, z), 0.012, 16)
        core.assign(r, M['black'])
        out.append(r)
        for y in (-BANNER_W / 2 - 0.035, BANNER_W / 2 + 0.035):
            cap = parts.rod('cap', (0.0, y - 0.012, z), (0.0, y + 0.012, z), 0.016, 16)
            core.assign(cap, M['copper'])
            out.append(cap)
    for y in (-BANNER_W / 2 + 0.05, BANNER_W / 2 - 0.05):
        c = parts.rod('cable', (0.0, y, z1), (0.0, y, 0.0), 0.0012, 6)
        core.assign(c, M['black'])
        out.append(c)
    return parts.finish(out, 'SM_EC_Banner', 30.0)


def stage(M):
    """The final table's stage, its front edge at x = 0, 10 m wide, 5.6 m deep."""
    out, lit = [], []
    W2, D, H = 5.0, 5.6, 0.45
    out.append(box('deck', 0.0, D, -W2, W2, H - 0.03, H, M['deck']))
    out.append(box('body', 0.04, D, -W2, W2, 0.0, H - 0.03, M['black']))
    out.append(box('fascia', 0.0, 0.04, -W2, W2, 0.0, H - 0.05, M['blackglass']))
    out.append(box('nosing', -0.02, 0.04, -W2, W2, H - 0.05, H, M['copper'], 0.006))
    lit.append(lit_box('edge', -0.012, 0.0, -W2, W2, H - 0.075, H - 0.06))
    lit.append(lit_box('toe', -0.012, 0.0, -W2, W2, 0.02, 0.03))
    # Steps at both ends, an LED line under each nosing.
    for sy in (-1, 1):
        for k in range(2):
            z = H * (k + 1) / 3
            y0, y1 = sy * (W2 + 0.35 * (2 - k)), sy * W2
            out.append(box('step', 0.6, 2.2, min(y0, y1), max(y0, y1), 0.0, z, M['black']))
            out.append(box('tread', 0.6, 2.2, min(y0, y1), max(y0, y1), z - 0.02, z, M['deck']))
            yl = y0 + sy * 0.004
            lit.append(lit_box('step_led', 0.62, 2.18, min(yl, yl + sy * 0.01), max(yl, yl + sy * 0.01), z - 0.04, z - 0.03))
    # The pylons: black towers with an LED face toward the room, either side of the backdrop.
    for sy in (-1, 1):
        y = sy * 4.55
        out.append(box('pylon', 3.6, 4.2, y - 0.3, y + 0.3, H, 4.15, M['black'], 0.01))
        for dy in (-0.16, 0.16):
            lit.append(lit_box('pylon_led', 3.586, 3.6, y + dy - 0.015, y + dy + 0.015, H + 0.3, 3.95))
        out.append(box('pylon_cap', 3.56, 4.24, y - 0.34, y + 0.34, 4.15, 4.2, M['copper'], 0.004))
    # The truss across the front of the stage, its lights pointed in.
    for dx, dz in ((0.0, 0.0), (0.35, 0.0), (0.0, 0.35), (0.35, 0.35)):
        r = parts.rod('chord', (1.0 + dx, -W2 + 0.2, 3.9 + dz), (1.0 + dx, W2 - 0.2, 3.9 + dz), 0.024, 10)
        core.assign(r, M['black'])
        out.append(r)
    n = 28
    for k in range(n + 1):
        y = -W2 + 0.2 + (2 * W2 - 0.4) * k / n
        for (a, b) in (((1.0, 3.9), (1.35, 4.25)), ((1.35, 3.9), (1.0, 4.25))):
            r = parts.rod('web', (a[0], y, a[1]), (b[0], y, b[1]), 0.008, 6)
            core.assign(r, M['black'])
            out.append(r)
    for k in range(6):
        y = -3.5 + k * 1.4
        can = parts.rod('fixture', (1.1, y, 3.78), (0.95, y, 3.55), 0.07, 16)
        core.assign(can, M['black'])
        out.append(can)
    return parts.finish(out, 'SM_EC_Stage', 30.0), parts.glow_object(lit, 'SM_EC_Stage_Glow')


def build():
    core.reset()
    M = mats()
    objs = []
    for make in (wall, column, ceiling, pendant, wall_light):
        objs += list(make(M))
    objs.append(table_sign(M))
    objs.append(banner(M))
    objs += list(stage(M))
    return objs


def bake_parts(objs):
    # The glow meshes are shaded by the game (a color it sets): exported unbaked.
    return [o for o in objs if not o.name.endswith('_Glow')]
