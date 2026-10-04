"""The Embercrest Casino's card room: a modular kit for the old showroom of a riverboat casino moored for good in 1994.

The game assembles the room from these pieces (BackRoomStageCardRoom.cpp), each modeled once and instanced:
- SM_CR_Wall: a 4 m bay of wall. Walnut wainscot with raised panels to the chair rail, a brass inlay in the rail,
  burgundy damask above in a gilt frame, a crown molding.
- SM_CR_Column: a panelled walnut column with mirrored upper panels and brass bands.
- SM_CR_Ceiling: one 4 m coffer of the ceiling. Dark walnut beams, a stepped gilt molding, a deep teal acoustic
  panel, a brass rosette around a recessed downlight (its glow is SM_CR_Ceiling_Glow).
- SM_CR_Pendant: the billiard light over every table, three green glass shades on a brass bar hung from the
  ceiling on two rods (SM_CR_Pendant_Glow: the bulbs and the shades' white insides).
- SM_CR_Sconce: a brass wall sconce with two frosted tulip shades (SM_CR_Sconce_Glow).
- SM_CR_Stage: the showroom's proscenium. A gilt arch, red velvet curtains gathered to the sides under a swag,
  the stage with steps at both ends, a black backdrop. The stream table stands on it (SM_CR_Stage_Glow: the
  footlights along its edge).
- SM_CR_Desk: the tournament desk. A walnut counter with two windows between glass partitions, a backlit sign
  box (the game writes on it), a whiteboard for the cash list, a queue of brass stanchions.
- SM_CR_Cage: the cashier's cage. A counter under a brass grille with three windows, frosted glass above, a sign
  box.
- SM_CR_Bar: the rail bar. A walnut counter with a black granite top and a brass foot rail, stools, a mirrored
  back bar with lit glass shelves of bottles (SM_CR_Bar_Glow).
- SM_CR_RiverDoors: a 4 m bay of brass-framed glass doors onto the river deck: wet planks, a railing, a bench,
  a vending machine with one dead row, deck lamps (SM_CR_RiverDoors_Glow).

Coordinates (meters): every piece stands on the floor (Z up) with its face toward -X, its width along Y, centered
on its origin. The stage's origin is the middle of its front edge; it extends toward +X. The ceiling coffer hangs
from z = 0 (the beams' undersides) upward; the pendant hangs from z = 2.0 (the ceiling) down to its shades at 0.
"""
import math
import random

import bpy  # noqa: F401
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import noise, object_coords, ramp, scaled, vec

MESHES = ['SM_CR_Wall', 'SM_CR_Column', 'SM_CR_Ceiling', 'SM_CR_Ceiling_Glow', 'SM_CR_Pendant', 'SM_CR_Pendant_Glow', 'SM_CR_Sconce',
          'SM_CR_Sconce_Glow', 'SM_CR_Stage', 'SM_CR_Stage_Glow', 'SM_CR_Desk', 'SM_CR_Desk_Glow', 'SM_CR_Cage', 'SM_CR_Cage_Glow', 'SM_CR_Bar',
          'SM_CR_Bar_Glow', 'SM_CR_RiverDoors', 'SM_CR_RiverDoors_Glow']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'cr_walnut': 2048, 'cr_damask': 2048, 'cr_gilt': 1024, 'cr_brass': 1024, 'cr_acoustic': 1024, 'cr_velvet': 2048,
                 'cr_backdrop': 512, 'cr_mirror': 512, 'cr_granite': 1024, 'cr_glass': 256, 'cr_frosted': 512, 'cr_green_glass': 512,
                 'cr_shade_white': 256, 'cr_leather': 512, 'cr_bottles': 1024, 'cr_planks': 2048, 'cr_iron': 512, 'cr_vending': 1024,
                 'cr_whiteboard': 512, 'cr_paint': 512, 'cr_stage_floor': 1024}
AO_DISTANCE = 0.25
REVIEW_VIEWS = [('front', -90, 8, 9.0, (0.0, 0.0, 1.8)), ('angle', -55, 18, 9.0, (0.0, 0.0, 1.8)), ('detail', -70, 10, 3.0, (0.0, 0.0, 1.2))]
REVIEW_SCREEN_LIGHT = False

WALL_W = 4.0
WALL_H = 4.2
RAIL_Z = 1.1
RND = random.Random(1994)


# ------------------------------------------------------------------ materials

def walnut_material(name='cr_walnut'):
    """Walnut panelling, a satin finish: long grain along Z or Y (the object's longest axis reads as grain either
    way at this scale), figure and pores, darker in the recesses."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    grain = noise(m, scaled(m, sep, 40.0, 40.0, 2.5), 1.0, 5.0, 0.6, 1.5)
    figure = noise(m, scaled(m, sep, 6.0, 6.0, 0.8), 1.0, 3.0)
    base = m.mix(ramp(m, grain, 0.35, 0.75), core.hex_linear(0x3a2214), core.hex_linear(0x5c3a22))
    base = m.mix(m.math('MULTIPLY', ramp(m, figure, 0.4, 0.8), 0.45), base, core.hex_linear(0x6d4a2c))
    pores = ramp(m, noise(m, scaled(m, sep, 700.0, 700.0, 30.0), 1.0, 2.0), 0.62, 0.72)
    base = m.mix(m.math('MULTIPLY', pores, 0.35), base, core.hex_linear(0x1d110a))
    m.set('Base Color', base)
    m.set('Roughness', m.math('ADD', 0.36, m.math('MULTIPLY', noise(m, o, 7.0, 3.0), 0.12)))
    m.set('Coat Weight', 0.25)
    bump = m.node('ShaderNodeBump', Strength=0.12, Distance=0.0006)
    m.link(m.math('ADD', grain, pores), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def damask_material():
    """Burgundy damask: a woven medallion lattice, satin figure on a matte ground, so it turns with the light."""
    m = core.Mat('cr_damask')
    tc, sep = object_coords(m)
    y, z = sep.outputs['Y'], sep.outputs['Z']
    period = 0.32
    k = 2 * math.pi / period
    a = m.math('SINE', m.math('MULTIPLY', m.math('ADD', y, z), k))
    b = m.math('SINE', m.math('MULTIPLY', m.math('SUBTRACT', y, z), k))
    c = m.math('COSINE', m.math('MULTIPLY', y, 2 * k))
    d = m.math('COSINE', m.math('MULTIPLY', z, 2 * k))
    lattice = m.math('ADD', m.math('MULTIPLY', a, b), m.math('MULTIPLY', m.math('MULTIPLY', c, d), 0.6))
    figure = ramp(m, lattice, 0.15, 0.45)
    heather = noise(m, tc.outputs['Object'], 400.0, 2.0)
    ground = m.mix(m.math('MULTIPLY', heather, 0.3), core.hex_linear(0x4a0d16), core.hex_linear(0x5a1420))
    m.set('Base Color', m.mix(figure, ground, core.hex_linear(0x7a2030)))
    m.set('Roughness', m.math('SUBTRACT', 0.82, m.math('MULTIPLY', figure, 0.32)))
    m.set('Sheen Weight', 0.6)
    m.set('Sheen Roughness', 0.4)
    bump = m.node('ShaderNodeBump', Strength=0.15, Distance=0.0004)
    m.link(m.math('ADD', figure, m.math('MULTIPLY', heather, 0.3)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def gilt_material():
    """Gilded molding: warm gold, rubbed back to red bole on the high points, dark in the recesses."""
    m = core.Mat('cr_gilt')
    tc, sep = object_coords(m)
    rub = ramp(m, noise(m, tc.outputs['Object'], 18.0, 4.0), 0.62, 0.8)
    m.set('Base Color', m.mix(m.math('MULTIPLY', rub, 0.4), core.hex_linear(0xc89a48), core.hex_linear(0x7a3a22)))
    m.set('Metallic', m.math('SUBTRACT', 1.0, m.math('MULTIPLY', rub, 0.6)))
    m.set('Roughness', m.math('ADD', 0.28, m.math('MULTIPLY', noise(m, tc.outputs['Object'], 90.0, 3.0), 0.15)))
    return m


def velvet_material(name='cr_velvet', hex_color=0x6e0c18):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    crush = noise(m, scaled(m, sep, 4.0, 4.0, 1.5), 1.0, 4.0)
    m.set('Base Color', m.mix(m.math('MULTIPLY', crush, 0.5), core.hex_linear(hex_color), core.hex_linear(0x3a0409)))
    m.set('Roughness', 0.9)
    m.set('Sheen Weight', 1.0)
    m.set('Sheen Roughness', 0.35)
    m.set('Sheen Tint', core.hex_linear(0xff7a8a))
    return m


def acoustic_material():
    m = core.Mat('cr_acoustic')
    tc, _ = object_coords(m)
    fiss = ramp(m, noise(m, tc.outputs['Object'], 140.0, 3.0, 0.7, 2.0), 0.55, 0.7)
    m.set('Base Color', m.mix(m.math('MULTIPLY', fiss, 0.6), core.hex_linear(0x15302e), core.hex_linear(0x0a1817)))
    m.set('Roughness', 0.95)
    bump = m.node('ShaderNodeBump', Strength=0.4, Distance=0.002)
    m.link(fiss, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def granite_material():
    m = core.Mat('cr_granite')
    tc, _ = object_coords(m)
    o = tc.outputs['Object']
    fleck = ramp(m, noise(m, o, 220.0, 2.0), 0.6, 0.66)
    mica = ramp(m, noise(m, o, 400.0, 1.0), 0.7, 0.74)
    base = m.mix(fleck, core.hex_linear(0x0b0b0c), core.hex_linear(0x3a3734))
    m.set('Base Color', m.mix(mica, base, core.hex_linear(0x8a8478)))
    m.set('Roughness', m.math('ADD', 0.08, m.math('MULTIPLY', noise(m, o, 30.0, 3.0), 0.08)))
    return m


def planks_material():
    """The river deck: weathered planks, wet from the rain, dark between the boards."""
    m = core.Mat('cr_planks')
    tc, sep = object_coords(m)
    x = sep.outputs['X']
    board = m.math('FRACT', m.math('DIVIDE', x, 0.145))
    gap = m.math('MAXIMUM', ramp(m, board, 0.06, 0.0), ramp(m, board, 0.94, 1.0))
    grain = noise(m, scaled(m, sep, 30.0, 1.2, 30.0), 1.0, 4.0, 0.6, 1.0)
    wet = ramp(m, noise(m, tc.outputs['Object'], 0.8, 3.0), 0.4, 0.7)
    base = m.mix(ramp(m, grain, 0.3, 0.8), core.hex_linear(0x3a3026), core.hex_linear(0x5d5040))
    base = m.mix(m.math('MULTIPLY', wet, 0.5), base, core.hex_linear(0x1c1712))
    m.set('Base Color', m.mix(gap, base, core.hex_linear(0x080706)))
    m.set('Roughness', m.math('SUBTRACT', 0.8, m.math('MULTIPLY', wet, 0.62)))
    bump = m.node('ShaderNodeBump', Strength=0.5, Distance=0.004)
    m.link(m.math('SUBTRACT', 1.0, gap), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def bottles_material():
    """Bottles behind the bar: glass in amber, green and clear by height band, labels on some."""
    m = core.Mat('cr_bottles')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    pick = noise(m, scaled(m, sep, 3.0, 3.0, 0.2), 1.0, 1.0)
    glass = m.mix(ramp(m, pick, 0.45, 0.5), core.hex_linear(0x6b3a0e), core.hex_linear(0x1c4a22))
    glass = m.mix(ramp(m, pick, 0.6, 0.65), glass, core.hex_linear(0xb8c4c0))
    m.set('Base Color', glass)
    m.set('Roughness', m.math('ADD', 0.05, m.math('MULTIPLY', noise(m, o, 40.0, 2.0), 0.08)))
    m.set('Specular IOR Level', 0.7)
    return m


def mats():
    M = {
        'walnut': walnut_material(),
        'damask': damask_material(),
        'gilt': gilt_material(),
        'brass': parts.metal('cr_brass', 0xb8913f, 0.22),
        'acoustic': acoustic_material(),
        'velvet': velvet_material(),
        'backdrop': velvet_material('cr_backdrop', 0x0b0b0e),
        'mirror': parts.gloss('cr_mirror', 0x9aa0a6, 0.02, metal=1.0),
        'granite': granite_material(),
        'glass': parts.gloss('cr_glass', 0x1a2224, 0.03),
        'frosted': parts.gloss('cr_frosted', 0xdad7cf, 0.45),
        'green_glass': parts.gloss('cr_green_glass', 0x0d4a26, 0.06, coat=0.6),
        'shade_white': parts.plastic('cr_shade_white', 0xf2ece0, 0.4, scuffs=0.0),
        'leather': parts.plastic('cr_leather', 0x3a0f12, 0.55, scuffs=0.4),
        'bottles': bottles_material(),
        'planks': planks_material(),
        'iron': parts.metal('cr_iron', 0x1a1a1c, 0.55),
        'vending': parts.plastic('cr_vending', 0x9c1a1a, 0.35, scuffs=0.6),
        'whiteboard': parts.gloss('cr_whiteboard', 0xf4f4f2, 0.12),
        'paint': parts.plastic('cr_paint', 0x15151a, 0.6, scuffs=0.3),
        'stage_floor': walnut_material('cr_stage_floor'),
    }
    return M


def glow(name):
    m = core.Mat(name)
    m.set('Base Color', (1.0, 1.0, 1.0, 1.0))
    m.set('Emission Color', (1.0, 1.0, 1.0, 1.0))
    m.set('Emission Strength', 1.0)
    return m


# ------------------------------------------------------------------ shapes

def box(name, x0, x1, y0, y1, z0, z1, mat, r=0.0):
    o = parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), r, 2)
    if mat is not None:
        core.assign(o, mat)
    return o


def frame_rect(name, x, y0, y1, z0, z1, w, d, mat):
    """A rectangular molding frame standing on the wall face (x), w wide, d proud."""
    return [box(name, x - d, x, y0, y1, z0, z0 + w, mat, d * 0.4), box(name, x - d, x, y0, y1, z1 - w, z1, mat, d * 0.4),
            box(name, x - d, x, y0, y0 + w, z0, z1, mat, d * 0.4), box(name, x - d, x, y1 - w, y1, z0, z1, mat, d * 0.4)]


def drape(name, y0, y1, z0, z1, x, folds, depth, mat, gather=1.0, seg_y=64, seg_z=24):
    """A hanging curtain: a sheet in the YZ plane at x, pleated in vertical folds that deepen toward the bottom
    (gather < 1 bunches the top)."""
    import bmesh
    bm = bmesh.new()
    rows = []
    for j in range(seg_z + 1):
        t = j / seg_z
        z = z0 + (z1 - z0) * t
        row = []
        for i in range(seg_y + 1):
            u = i / seg_y
            y = y0 + (y1 - y0) * u
            amp = depth * (0.55 + 0.45 * (1.0 - t)) * (gather if t > 0.85 else 1.0)
            dx = math.sin(u * folds * 2 * math.pi + 0.7 * math.sin(u * 11.0)) * amp
            row.append(bm.verts.new((x + dx, y, z)))
        rows.append(row)
    for j in range(seg_z):
        for i in range(seg_y):
            bm.faces.new((rows[j][i], rows[j][i + 1], rows[j + 1][i + 1], rows[j + 1][i]))
    o = core.mesh_object(name, bm)
    core.orient_normals(o, Vector((-1.0, 0.0, 0.0)))
    core.assign(o, mat)
    return o


# ------------------------------------------------------------------ the pieces

def wall(M):
    out = []
    hw = WALL_W / 2
    # The wall itself (damask; behind the wainscot it doesn't show), the baseboard, the wainscot field.
    out.append(box('wall', 0.0, 0.12, -hw, hw, 0.0, WALL_H, M['damask']))
    out.append(box('base', -0.028, 0.0, -hw, hw, 0.0, 0.2, M['walnut'], 0.004))
    out.append(box('wainscot', -0.02, 0.0, -hw, hw, 0.2, RAIL_Z - 0.04, M['walnut']))
    # Two raised panels per bay, with a bolection molding around each.
    for c in (-hw / 2, hw / 2):
        pw, ph = 1.62, 0.66
        zc = 0.2 + (RAIL_Z - 0.24) / 2
        out.append(box('panel', -0.042, -0.02, c - pw / 2, c + pw / 2, zc - ph / 2, zc + ph / 2, M['walnut'], 0.01))
        out += frame_rect('bolection', -0.02, c - pw / 2 - 0.03, c + pw / 2 + 0.03, zc - ph / 2 - 0.03, zc + ph / 2 + 0.03, 0.03, 0.03, M['walnut'])
    # The chair rail: a walnut cap with a brass inlay.
    out.append(box('rail', -0.055, 0.0, -hw, hw, RAIL_Z - 0.05, RAIL_Z + 0.02, M['walnut'], 0.008))
    out.append(box('inlay', -0.057, -0.05, -hw, hw, RAIL_Z - 0.022, RAIL_Z - 0.01, M['brass']))
    # The damask in a gilt frame, and the crown.
    out += frame_rect('gilt', 0.0, -hw + 0.18, hw - 0.18, RAIL_Z + 0.22, WALL_H - 0.5, 0.045, 0.028, M['gilt'])
    out.append(box('frieze', -0.03, 0.0, -hw, hw, WALL_H - 0.36, WALL_H - 0.16, M['walnut']))
    out.append(box('crown', -0.09, 0.0, -hw, hw, WALL_H - 0.16, WALL_H - 0.06, M['walnut'], 0.02))
    out.append(box('bead', -0.1, -0.08, -hw, hw, WALL_H - 0.13, WALL_H - 0.11, M['gilt']))
    out.append(box('cap', -0.12, 0.0, -hw, hw, WALL_H - 0.06, WALL_H, M['walnut'], 0.01))
    return parts.finish(out, 'SM_CR_Wall', 30.0)


def column(M):
    out = []
    s = 0.35
    out.append(box('plinth', -s - 0.04, s + 0.04, -s - 0.04, s + 0.04, 0.0, 0.24, M['walnut'], 0.01))
    out.append(box('shaft', -s, s, -s, s, 0.24, WALL_H - 0.3, M['walnut']))
    out.append(box('band', -s - 0.012, s + 0.012, -s - 0.012, s + 0.012, RAIL_Z - 0.03, RAIL_Z + 0.01, M['brass'], 0.004))
    out.append(box('band', -s - 0.012, s + 0.012, -s - 0.012, s + 0.012, WALL_H - 0.66, WALL_H - 0.62, M['brass'], 0.004))
    # Mirrored panels on all four faces between the bands, set in walnut.
    for ang in (0, 90, 180, 270):
        R = parts.rot_z(ang)
        mir = box('mirror', -s - 0.006, -s + 0.002, -s + 0.07, s - 0.07, RAIL_Z + 0.12, WALL_H - 0.76, M['mirror'])
        mir.data.transform(R)
        out.append(mir)
        for o in frame_rect('stile', -s - 0.002, -s + 0.05, s - 0.05, RAIL_Z + 0.1, WALL_H - 0.74, 0.03, 0.016, M['gilt']):
            o.data.transform(R)
            out.append(o)
    out.append(box('capital', -s - 0.08, s + 0.08, -s - 0.08, s + 0.08, WALL_H - 0.3, WALL_H - 0.12, M['walnut'], 0.02))
    out.append(box('abacus', -s - 0.12, s + 0.12, -s - 0.12, s + 0.12, WALL_H - 0.12, WALL_H, M['walnut'], 0.015))
    out.append(box('bead', -s - 0.09, s + 0.09, -s - 0.09, s + 0.09, WALL_H - 0.31, WALL_H - 0.28, M['gilt']))
    return parts.finish(out, 'SM_CR_Column', 30.0)


def ceiling(M):
    out, lit = [], []
    h = 2.0
    beam = 0.14  # half a beam on each edge: 28 cm where two coffers meet
    depth = 0.32
    # Beams round the edge (their undersides at z = 0) and the coffer's sides.
    for sx in (-1, 1):
        out.append(box('beam', sx * h - (beam if sx > 0 else 0.0), sx * h + (0.0 if sx > 0 else beam), -h, h, 0.0, depth, M['walnut']))
        out.append(box('beam', -h, h, sx * h - (beam if sx > 0 else 0.0), sx * h + (0.0 if sx > 0 else beam), 0.0, depth, M['walnut']))
    # A stepped gilt molding inside the coffer, then the acoustic panel at the top.
    for k, (inset, z) in enumerate(((beam, 0.08), (beam + 0.06, 0.18))):
        a = h - inset
        out += [box('mold', -a, a, -a, -a + 0.05, z, z + 0.05, M['gilt']), box('mold', -a, a, a - 0.05, a, z, z + 0.05, M['gilt']),
                box('mold', -a, -a + 0.05, -a, a, z, z + 0.05, M['gilt']), box('mold', a - 0.05, a, -a, a, z, z + 0.05, M['gilt'])]
    out.append(box('panel', -h + beam, h - beam, -h + beam, h - beam, depth, depth + 0.02, M['acoustic']))
    # The rosette and the downlight's trim.
    ros = parts.lathe_at('rosette', [(0.0, depth), (0.34, depth), (0.33, depth - 0.02), (0.26, depth - 0.035), (0.2, depth - 0.03), (0.17, depth - 0.045),
                                     (0.12, depth - 0.045)], Matrix(), 48)
    core.assign(ros, M['brass'])
    out.append(ros)
    trim = parts.lathe_at('trim', [(0.12, depth - 0.045), (0.105, depth - 0.05), (0.09, depth - 0.02)], Matrix(), 40)
    core.assign(trim, M['brass'])
    out.append(trim)
    disc = parts.lathe_at('downlight', [(0.0, depth - 0.022), (0.09, depth - 0.022)], Matrix(), 32)
    lit.append(disc)
    return parts.finish(out, 'SM_CR_Ceiling', 30.0), parts.glow_object(lit, 'SM_CR_Ceiling_Glow')


def pendant(M):
    out, lit = [], []
    top = 2.0
    bar_z = 0.24
    span = 1.8
    out.append(box('bar', -0.03, 0.03, -span / 2, span / 2, bar_z - 0.025, bar_z + 0.025, M['brass'], 0.01))
    for y in (-span / 2 - 0.01, span / 2 + 0.01):
        cap = parts.lathe_at('endcap', [(0.0, -0.035), (0.035, -0.03), (0.04, 0.0), (0.035, 0.03), (0.0, 0.035)],
                             Matrix.Translation((0.0, y, bar_z)) @ parts.rot_x(-90), 16)
        core.assign(cap, M['brass'])
        out.append(cap)
    for y in (-0.62, 0.62):
        out.append(parts.rod('rod', (0.0, y, bar_z + 0.02), (0.0, y, top - 0.04), 0.008, 12))
        core.assign(out[-1], M['brass'])
        canopy = parts.lathe_at('canopy', [(0.0, top - 0.06), (0.03, top - 0.06), (0.065, top - 0.02), (0.07, top), (0.0, top)], Matrix.Translation((0.0, y, 0.0)), 32)
        core.assign(canopy, M['brass'])
        out.append(canopy)
    for y in (-0.6, 0.0, 0.6):
        T = Matrix.Translation((0.0, y, 0.0))
        # The shade: green cased glass outside, white inside; a brass cap and neck up to the bar.
        outer = parts.lathe_at('shade', [(0.205, 0.0), (0.2, 0.02), (0.17, 0.08), (0.11, 0.15), (0.06, 0.18)], T, 48)
        core.assign(outer, M['green_glass'])
        inner = parts.lathe_at('inner', [(0.055, 0.17), (0.105, 0.14), (0.165, 0.075), (0.195, 0.018), (0.2, 0.004)], T, 48)
        core.assign(inner, M['shade_white'])
        rim = parts.lathe_at('rim', [(0.2, -0.004), (0.208, 0.0), (0.2, 0.006)], T, 48)
        core.assign(rim, M['brass'])
        cap = parts.lathe_at('cap', [(0.065, 0.175), (0.06, 0.2), (0.03, 0.215), (0.0, 0.22)], T, 32)
        core.assign(cap, M['brass'])
        neck = parts.rod('neck', (0.0, y, 0.215), (0.0, y, bar_z - 0.02), 0.012, 12)
        core.assign(neck, M['brass'])
        out += [outer, inner, rim, cap, neck]
        bulb = parts.lathe_at('bulb', [(0.0, 0.06), (0.03, 0.065), (0.04, 0.09), (0.03, 0.12), (0.015, 0.135), (0.0, 0.14)], T, 24)
        glowdisc = parts.lathe_at('glow', [(0.0, 0.02), (0.19, 0.02)], T, 40)
        lit += [bulb, glowdisc]
    return parts.finish(out, 'SM_CR_Pendant', 35.0), parts.glow_object(lit, 'SM_CR_Pendant_Glow')


def sconce(M):
    out, lit = [], []
    plate = parts.lathe_at('plate', [(0.0, -0.002), (0.07, -0.002), (0.075, 0.008), (0.06, 0.02), (0.0, 0.022)], parts.rot_y(-90), 32)
    plate.data.transform(Matrix.Translation((0.0, 0.0, 1.95)))
    core.assign(plate, M['brass'])
    out.append(plate)
    for side in (-1, 1):
        arm = parts.tube('arm', [(-0.02, 0.0, 1.95), (-0.1, side * 0.06, 1.93), (-0.16, side * 0.16, 1.98), (-0.17, side * 0.18, 2.05)], 0.008, 10, 8)
        core.assign(arm, M['brass'])
        cup = parts.lathe_at('cup', [(0.0, 2.04), (0.03, 2.045), (0.035, 2.07)], Matrix.Translation((-0.17, side * 0.18, 0.0)), 24)
        core.assign(cup, M['brass'])
        tulip = parts.lathe_at('tulip', [(0.03, 2.07), (0.05, 2.1), (0.075, 2.16), (0.08, 2.2), (0.072, 2.22)], Matrix.Translation((-0.17, side * 0.18, 0.0)), 32)
        core.assign(tulip, M['frosted'])
        out += [arm, cup, tulip]
        lit.append(parts.lathe_at('flame', [(0.0, 2.08), (0.02, 2.1), (0.03, 2.14), (0.0, 2.19)], Matrix.Translation((-0.17, side * 0.18, 0.0)), 16))
    return parts.finish(out, 'SM_CR_Sconce', 35.0), parts.glow_object(lit, 'SM_CR_Sconce_Glow')


def stage(M):
    out, lit = [], []
    W2, D, H = 5.0, 5.6, 0.45
    open_w, open_h = 10.0, 3.95
    # The stage: a walnut floor, a fascia with a gilt band, steps at both ends; the backdrop behind.
    out.append(box('deck', 0.0, D, -W2, W2, H - 0.04, H, M['stage_floor']))
    out.append(box('fascia', -0.02, 0.06, -W2, W2, 0.0, H - 0.04, M['walnut']))
    out.append(box('band', -0.035, -0.02, -W2, W2, H - 0.1, H - 0.075, M['gilt']))
    out.append(box('nosing', -0.05, 0.02, -W2, W2, H - 0.045, H, M['walnut'], 0.015))
    for s in (-1, 1):
        for k in range(3):
            z = H * (k + 1) / 4
            y_out = s * (W2 + 0.3 * (3 - k))
            out.append(box('step', 0.15, 1.65, min(s * W2, y_out), max(s * W2, y_out), 0.0, z, M['walnut'], 0.01))
        # A brass handrail down each flight.
        out.append(parts.rod('handrail', (1.7, s * (W2 + 0.95), 0.95), (1.7, s * W2, 0.95 + H), 0.022, 12))
        core.assign(out[-1], M['brass'])
        out.append(parts.rod('post', (1.7, s * (W2 + 0.95), 0.0), (1.7, s * (W2 + 0.95), 0.97), 0.025, 12))
        core.assign(out[-1], M['brass'])
    out.append(box('backdrop', D - 0.05, D, -W2 - 1.5, W2 + 1.5, H, WALL_H, M['backdrop']))
    # The proscenium: a wall around the opening (dark), and a deep gilt arch frame with a keystone cartouche.
    out.append(box('pros_l', -0.25, 0.0, -W2 - 1.8, -open_w / 2, 0.0, WALL_H, M['damask']))
    out.append(box('pros_r', -0.25, 0.0, open_w / 2, W2 + 1.8, 0.0, WALL_H, M['damask']))
    out.append(box('pros_t', -0.25, 0.0, -open_w / 2, open_w / 2, open_h, WALL_H, M['damask']))
    for k, (w, d) in enumerate(((0.36, 0.08), (0.24, 0.14), (0.12, 0.2))):
        y0, y1 = -open_w / 2 - w, open_w / 2 + w
        out.append(box('arch', -0.25 - d, -0.25, y0, y0 + w, 0.0, open_h + w, M['gilt'], 0.02))
        out.append(box('arch', -0.25 - d, -0.25, y1 - w, y1, 0.0, open_h + w, M['gilt'], 0.02))
        out.append(box('arch', -0.25 - d, -0.25, y0, y1, open_h, min(WALL_H - 0.02, open_h + w), M['gilt'], 0.02))
    cart = parts.lathe_at('cartouche', [(0.0, 0.0), (0.32, 0.0), (0.36, 0.06), (0.3, 0.12), (0.0, 0.16)], parts.rot_y(-90), 32)
    cart.data.transform(Matrix.Translation((-0.42, 0.0, open_h + 0.08)) @ Matrix.Scale(1.0, 4, (0, 0, 1)))
    core.assign(cart, M['gilt'])
    out.append(cart)
    # Curtains: gathered to each side of the opening, and a swag valance across the top.
    for s in (-1, 1):
        c = drape('curtain', min(s * open_w / 2, s * (open_w / 2 - 1.2)), max(s * open_w / 2, s * (open_w / 2 - 1.2)), H, open_h - 0.02, 0.15, 7, 0.07, M['velvet'], 0.5)
        out.append(c)
        tie = parts.rod('tieback', (0.08, s * (open_w / 2 - 1.25), 1.5), (0.08, s * (open_w / 2 - 0.05), 1.45), 0.02, 10)
        core.assign(tie, M['gilt'])
        out.append(tie)
    out.append(drape('valance', -open_w / 2, open_w / 2, open_h - 0.55, open_h - 0.02, 0.1, 22, 0.05, M['velvet'], 1.0, 96, 10))
    out.append(box('pelmet', -0.05, 0.12, -open_w / 2, open_w / 2, open_h - 0.12, open_h, M['gilt'], 0.01))
    # Footlights: small bulbs in a brass trough along the edge.
    out.append(box('trough', -0.12, 0.0, -W2 + 0.3, W2 - 0.3, H, H + 0.05, M['brass'], 0.01))
    for k in range(22):
        y = -W2 + 0.5 + k * (2 * W2 - 1.0) / 21
        lit.append(parts.lathe_at('foot', [(0.0, H + 0.03), (0.02, H + 0.04), (0.02, H + 0.06), (0.0, H + 0.075)], Matrix.Translation((-0.06, y, 0.0)), 12))
    return parts.finish(out, 'SM_CR_Stage', 30.0), parts.glow_object(lit, 'SM_CR_Stage_Glow')


def counter(M, name, length, windows, grille):
    """The desk and the cage share a counter: walnut front panels, a granite top, a transaction shelf; the desk
    has glass partitions between its windows, the cage a brass grille."""
    out, lit = [], []
    L2 = length / 2
    out.append(box('front', 0.0, 0.62, -L2, L2, 0.0, 1.06, M['walnut']))
    for k in range(int(length / 1.0)):
        y = -L2 + 0.5 + k * 1.0
        out.append(box('panel', -0.02, 0.0, y - 0.4, y + 0.4, 0.18, 0.92, M['walnut'], 0.008))
        out += frame_rect('mold', -0.02, y - 0.43, y + 0.43, 0.15, 0.95, 0.02, 0.015, M['gilt'])
    out.append(box('kick', -0.03, 0.0, -L2, L2, 0.0, 0.12, M['paint']))
    out.append(box('top', -0.08, 0.66, -L2 - 0.04, L2 + 0.04, 1.06, 1.1, M['granite'], 0.01))
    out.append(box('back', 1.5, 1.62, -L2 - 0.2, L2 + 0.2, 0.0, WALL_H - 0.4, M['walnut']))
    # The sign box over the windows (the game writes on its face).
    out.append(box('signbox', 1.25, 1.5, -1.6, 1.6, 2.35, 2.95, M['walnut'], 0.02))
    lit.append(box('signface', 1.235, 1.25, -1.5, 1.5, 2.42, 2.88, None))
    win_y = [(-L2 + (k + 0.5) * length / windows) for k in range(windows)]
    if grille:
        out.append(box('grille_top', -0.02, 0.02, -L2, L2, 2.36, 2.42, M['brass']))
        out.append(box('frosted', 0.0, 0.01, -L2, L2, 2.42, 3.2, M['frosted']))
        n = int(length / 0.08)
        for k in range(n + 1):
            y = -L2 + k * length / n
            if any(abs(y - wy) < 0.33 for wy in win_y):
                continue
            out.append(parts.rod('bar', (0.0, y, 1.1), (0.0, y, 2.36), 0.006, 8))
            core.assign(out[-1], M['brass'])
        for wy in win_y:
            out.append(box('arch', -0.02, 0.02, wy - 0.36, wy + 0.36, 1.82, 1.88, M['brass'], 0.01))
            out.append(box('slot', -0.06, 0.6, wy - 0.25, wy + 0.25, 1.1, 1.12, M['brass'], 0.005))
    else:
        for k in range(windows + 1):
            y = -L2 + k * length / windows
            out.append(box('glass', 0.15, 0.17, y - 0.01, y + 0.01, 1.1, 2.0, M['glass']))
            out.append(box('post', 0.13, 0.19, y - 0.02, y + 0.02, 1.1, 2.04, M['brass'], 0.01))
        out.append(box('rail', 0.13, 0.19, -L2, L2, 2.0, 2.04, M['brass'], 0.01))
    return out, lit


def desk(M):
    out, lit = counter(M, 'desk', 6.0, 2, False)
    # The whiteboard for the cash list, beside the windows on the back wall.
    out.append(box('board', 1.47, 1.5, 2.15, 3.35, 1.25, 2.25, M['whiteboard'], 0.005))
    out += frame_rect('board_frame', 1.47, 2.12, 3.38, 1.22, 2.28, 0.03, 0.02, M['brass'])
    out.append(box('tray', 1.4, 1.47, 2.2, 3.3, 1.2, 1.23, M['brass'], 0.005))
    # The queue in front: brass stanchions with a velvet rope.
    posts = [(-1.2, -2.4), (-1.2, -0.8), (-1.2, 0.8), (-1.2, 2.4)]
    for x, y in posts:
        out.append(parts.rod('post', (x, y, 0.0), (x, y, 0.95), 0.025, 16))
        core.assign(out[-1], M['brass'])
        base = parts.lathe_at('foot', [(0.0, 0.0), (0.16, 0.0), (0.15, 0.03), (0.04, 0.06), (0.0, 0.06)], Matrix.Translation((x, y, 0.0)), 24)
        core.assign(base, M['brass'])
        top = parts.lathe_at('knob', [(0.0, 0.94), (0.035, 0.96), (0.04, 0.99), (0.0, 1.03)], Matrix.Translation((x, y, 0.0)), 16)
        core.assign(top, M['brass'])
        out += [base, top]
    for (xa, ya), (xb, yb) in zip(posts[:-1], posts[1:]):
        mid = ((xa + xb) / 2, (ya + yb) / 2, 0.78)
        rope = parts.tube('rope', [(xa, ya, 0.9), mid, (xb, yb, 0.9)], 0.018, 10, 10)
        core.assign(rope, M['velvet'])
        out.append(rope)
    return parts.finish(out, 'SM_CR_Desk', 30.0), parts.glow_object(lit, 'SM_CR_Desk_Glow')


def cage(M):
    out, lit = counter(M, 'cage', 6.0, 3, True)
    return parts.finish(out, 'SM_CR_Cage', 30.0), parts.glow_object(lit, 'SM_CR_Cage_Glow')


def bar(M):
    out, lit = [], []
    L2 = 4.0
    out.append(box('front', 0.0, 0.6, -L2, L2, 0.0, 1.04, M['walnut']))
    for k in range(8):
        y = -L2 + 0.5 + k
        out.append(box('panel', -0.02, 0.0, y - 0.42, y + 0.42, 0.16, 0.92, M['walnut'], 0.01))
    out.append(box('top', -0.12, 0.64, -L2 - 0.08, L2 + 0.08, 1.04, 1.09, M['granite'], 0.02))
    out.append(box('armrest', -0.16, -0.06, -L2 - 0.08, L2 + 0.08, 1.06, 1.12, M['leather'], 0.04))
    out.append(parts.rod('footrail', (-0.22, -L2, 0.2), (-0.22, L2, 0.2), 0.025, 16))
    core.assign(out[-1], M['brass'])
    for k in range(9):
        y = -L2 + k * 1.0
        out.append(parts.rod('bracket', (-0.22, y, 0.2), (0.0, y, 0.25), 0.012, 8))
        core.assign(out[-1], M['brass'])
    # Stools in front.
    for k in range(7):
        y = -L2 + 0.6 + k * 1.13
        out.append(parts.rod('stool', (-0.7, y, 0.0), (-0.7, y, 0.72), 0.03, 16))
        core.assign(out[-1], M['brass'])
        base = parts.lathe_at('stool_base', [(0.0, 0.0), (0.22, 0.0), (0.21, 0.03), (0.0, 0.04)], Matrix.Translation((-0.7, y, 0.0)), 32)
        core.assign(base, M['brass'])
        ring = parts.lathe_at('stool_ring', [(0.17, 0.3), (0.185, 0.31), (0.17, 0.32)], Matrix.Translation((-0.7, y, 0.0)), 32)
        core.assign(ring, M['brass'])
        seat = parts.lathe_at('stool_seat', [(0.0, 0.72), (0.2, 0.72), (0.21, 0.76), (0.19, 0.8), (0.0, 0.81)], Matrix.Translation((-0.7, y, 0.0)), 32)
        core.assign(seat, M['leather'])
        out += [base, ring, seat]
    # The back bar: a mirror wall, glass shelves on brass brackets, bottles, lit from under each shelf.
    out.append(box('backbar', 1.3, 1.85, -L2, L2, 0.0, 0.95, M['walnut']))
    out.append(box('backtop', 1.25, 1.88, -L2, L2, 0.95, 0.99, M['granite'], 0.01))
    out.append(box('mirror', 1.86, 1.88, -L2 + 0.2, L2 - 0.2, 1.0, 2.9, M['mirror']))
    out += frame_rect('mirror_frame', 1.86, -L2 + 0.15, L2 - 0.15, 0.97, 2.95, 0.06, 0.04, M['gilt'])
    for k, z in enumerate((1.35, 1.8, 2.25)):
        out.append(box('shelf', 1.55, 1.85, -L2 + 0.3, L2 - 0.3, z, z + 0.015, M['glass']))
        lit.append(box('strip', 1.6, 1.84, -L2 + 0.32, L2 - 0.32, z - 0.006, z, None))
        n = 34
        for b in range(n):
            y = -L2 + 0.45 + b * (2 * L2 - 0.9) / (n - 1) + RND.uniform(-0.03, 0.03)
            hgt = RND.choice((0.24, 0.28, 0.3, 0.32, 0.22))
            rad = RND.choice((0.035, 0.04, 0.045))
            x = 1.7 + RND.uniform(-0.05, 0.05)
            bottle = parts.lathe_at('bottle', [(0.0, 0.0), (rad, 0.0), (rad, hgt * 0.62), (rad * 0.45, hgt * 0.8), (0.014, hgt * 0.88), (0.014, hgt), (0.0, hgt)],
                                    Matrix.Translation((x, y, z + 0.016)), 14)
            core.assign(bottle, M['bottles'])
            out.append(bottle)
    return parts.finish(out, 'SM_CR_Bar', 30.0), parts.glow_object(lit, 'SM_CR_Bar_Glow')


def river_doors(M):
    out, lit = [], []
    hw = WALL_W / 2
    # The bay: walnut wainscot returns either side, a transom, and two pairs of brass-framed glass doors.
    out.append(box('jamb', 0.0, 0.14, -hw, -hw + 0.18, 0.0, WALL_H, M['walnut']))
    out.append(box('jamb', 0.0, 0.14, hw - 0.18, hw, 0.0, WALL_H, M['walnut']))
    out.append(box('head', 0.0, 0.14, -hw, hw, 3.0, WALL_H, M['walnut']))
    out.append(box('transom_bar', -0.01, 0.07, -hw + 0.18, hw - 0.18, 2.62, 2.7, M['brass'], 0.01))
    out.append(box('transom', 0.02, 0.03, -hw + 0.18, hw - 0.18, 2.7, 3.0, M['glass']))
    for k in range(4):
        y0 = -hw + 0.18 + k * (WALL_W - 0.36) / 4
        y1 = y0 + (WALL_W - 0.36) / 4
        out += frame_rect('door', 0.07, y0, y1, 0.0, 2.62, 0.07, 0.07, M['brass'])
        out.append(box('glass', 0.025, 0.035, y0 + 0.07, y1 - 0.07, 0.07, 2.55, M['glass']))
        out.append(box('kick', 0.0, 0.07, y0 + 0.07, y1 - 0.07, 0.07, 0.3, M['brass'], 0.005))
        hy = y1 - 0.12 if k % 2 == 0 else y0 + 0.12
        out.append(parts.rod('pull', (-0.06, hy, 0.9), (-0.06, hy, 1.5), 0.016, 12))
        core.assign(out[-1], M['brass'])
    # The exit sign over the doors.
    out.append(box('exit', -0.08, 0.0, -0.3, 0.3, 3.2, 3.42, M['paint'], 0.01))
    lit.append(box('exit_face', -0.085, -0.08, -0.26, 0.26, 3.24, 3.38, None))
    # The deck outside: planks, a railing over the water, a bench, the vending machine, two lamps.
    out.append(box('deck', 0.14, 5.0, -4.0, 4.0, -0.08, -0.02, M['planks']))
    out.append(box('fascia', 4.95, 5.05, -4.0, 4.0, -0.4, -0.02, M['planks']))
    for k in range(17):
        y = -4.0 + k * 0.5
        out.append(parts.rod('baluster', (4.85, y, -0.02), (4.85, y, 1.0), 0.012, 8))
        core.assign(out[-1], M['iron'])
    out.append(box('cap', 4.78, 4.92, -4.0, 4.0, 1.0, 1.06, M['planks'], 0.01))
    out.append(box('midrail', 4.83, 4.87, -4.0, 4.0, 0.45, 0.48, M['iron']))
    out.append(box('bench_seat', 2.6, 3.05, -3.4, -1.6, 0.42, 0.47, M['planks'], 0.01))
    out.append(box('bench_back', 3.0, 3.06, -3.4, -1.6, 0.47, 0.9, M['planks'], 0.01))
    for y in (-3.3, -1.7):
        out.append(box('bench_leg', 2.65, 3.02, y - 0.03, y + 0.03, -0.02, 0.42, M['iron']))
    out.append(box('vending', 1.0, 1.8, 2.6, 3.6, -0.02, 1.85, M['vending'], 0.03))
    lit.append(box('vending_face', 0.995, 1.0, 2.7, 3.4, 0.7, 1.75, None))
    out.append(box('vending_tray', 0.98, 1.02, 2.75, 3.25, 0.18, 0.42, M['paint'], 0.01))
    for y in (-3.8, 3.8):
        out.append(parts.rod('lamp_post', (4.8, y, -0.02), (4.8, y, 2.4), 0.04, 12))
        core.assign(out[-1], M['iron'])
        lit.append(parts.lathe_at('lamp', [(0.0, 2.4), (0.12, 2.48), (0.16, 2.58), (0.12, 2.68), (0.0, 2.74)], Matrix.Translation((4.8, y, 0.0)), 24))
    out.append(parts.rod('ashtray', (0.6, -0.6, -0.02), (0.6, -0.6, 0.8), 0.05, 16))
    core.assign(out[-1], M['iron'])
    return parts.finish(out, 'SM_CR_RiverDoors', 30.0), parts.glow_object(lit, 'SM_CR_RiverDoors_Glow')


def build():
    core.reset()
    M = mats()
    objs = [wall(M), column(M)]
    for make in (ceiling, pendant, sconce, stage, desk, cage, bar, river_doors):
        objs += list(make(M))
    return objs


def bake_parts(objs):
    # The glow meshes are shaded by the game (a color it sets): exported unbaked.
    return [o for o in objs if not o.name.endswith('_Glow')]
