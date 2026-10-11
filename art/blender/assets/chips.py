"""SM_ChipStacks: a few stacks of clay chips from the Tuesday game at the laundromat; and the Embercrest's tournament set.

Four denominations of 39 mm, 10 g-style clay chips:
- edge inserts that wrap into the face;
- a molded ring;
- a printed inlay from the "Spin Cycle Club".

Each denomination is baked once, then copied into the stacks with a little jitter (assemble), so
every chip is crisp and the texture budget stays at four small sets. Faces hidden inside a stack
are removed.

The Embercrest's tournament chips (next_segment_spec 6.2 B), SM_Chip_T100, _T500, _T1K, _T5K and _T25K: the same 39 x 3.3 mm
clay, in the North American tournament colours (100 black, 500 purple, 1,000 yellow, 5,000 orange, 25,000 ash with ember
edge spots), six two-tone edge spots each, and an inlay printed with the Embercrest's crest (brand.py's mark), the room's
name and the value. They are drawn by the hundreds, so they are low (384 triangles: no inlay step, no extra rings): the
inlay's lip, the molded ring and the clay's grain are in the normal map. SM_ChipStacks_EC is a Nightly's starting stack
(10 x 100, 8 x 500, 15 x 1,000) and a Big Stack's two 5,000s, in 20-high columns, big chips in front.

Coordinates (meters): origin on the desk at the middle of the group, Z up; the group faces -Y.
SM_Chip_<denomination> are the single chips, origin at the bottom center.
"""
import math
import os
import random
import sys

import bpy  # noqa: I001 (bpy first: the PyPI build registers bmesh and mathutils on import)
import bmesh
from mathutils import Matrix, Vector

from artkit import core
from artkit.shade import image_surface, mix_float, noise, object_coords, planar, ramp, smooth_less
from artkit.sheet import Sheet

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import brand  # noqa: E402

TOURNAMENT = ['T100', 'T500', 'T1K', 'T5K', 'T25K']
MESHES = ['SM_ChipStacks', 'SM_Chip_1', 'SM_Chip_5', 'SM_Chip_25', 'SM_Chip_100', 'SM_ChipStacks_EC'] + [f'SM_Chip_{t}' for t in TOURNAMENT]
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
AO_DISTANCE = 0.003

R = 0.0195
T = 0.0033
INLAY_R = 0.0125
RING_R = 0.0163
# Denomination: (chip color, insert color, number color).
CHIPS = {
    '1': (0xe8e4da, 0x2a5bb8, 0x2a5bb8),
    '5': (0xb01f2a, 0xf1ece0, 0xb01f2a),
    '25': (0x1c7443, 0xf1ece0, 0x1c7443),
    '100': (0x161617, 0xe9c46a, 0x161617),
}
# Stacks: (denomination, count, x, y). Loose chips lie flat beside them.
STACKS = [('5', 12, 0.0, 0.0), ('25', 8, 0.0405, 0.0105), ('100', 5, 0.0165, 0.0385), ('1', 1, -0.0335, -0.028), ('1', 1, 0.066, -0.016)]


# Lathe profile (radius, z): the face dips 0.1 mm into the inlay; the edge is softly rounded.
PROFILE = [(0.0, 0.0001), (INLAY_R, 0.0001), (INLAY_R + 0.0001, 0.0), (R - 0.0012, 0.0), (R - 0.0003, 0.0003), (R, 0.0009),
           (R, T - 0.0009), (R - 0.0003, T - 0.0003), (R - 0.0012, T), (INLAY_R + 0.0001, T), (INLAY_R, T - 0.0001), (0.0, T - 0.0001)]


def spade(s, cx, cy, size, color):
    s.circle(cx - 0.27 * size, cy - 0.02 * size, 0.28 * size, color, n=48, rough=0.4)
    s.circle(cx + 0.27 * size, cy - 0.02 * size, 0.28 * size, color, n=48, rough=0.4)
    s.poly([(cx, cy + 0.62 * size), (cx - 0.52 * size, cy + 0.02 * size), (cx + 0.52 * size, cy + 0.02 * size)], color, rough=0.4)
    s.poly([(cx, cy - 0.05 * size), (cx + 0.2 * size, cy - 0.55 * size), (cx - 0.2 * size, cy - 0.55 * size)], color, rough=0.4)


def inlay_sheet(denom):
    """The printed inlay (millimeters, 25 x 25, centered)."""
    base, insert, ink = CHIPS[denom]
    d = INLAY_R * 2000
    s = Sheet(d, d)
    c = d / 2
    s.circle(c, c, c, 0xf2ede0, n=128, rough=0.35)
    s.ring(c, c, c - 1.1, c, base, rough=0.35)
    s.ring(c, c, c - 1.7, c - 1.45, base, rough=0.35)
    s.text(denom, c, c - 3.3, 9.0 if len(denom) < 3 else 7.2, ink, face='Black', align='CENTER', rough=0.35)
    s.text_on_arc('SPIN CYCLE', c, c, 8.4, 2.1, 0x2d2d2d, face='Bold', tracking=1.15, rough=0.4)
    spade(s, c, c - 7.6, 2.4, 0x2d2d2d)
    return s.render(f'chip_{denom}_inlay', 512)


def chip_material(denom, inlay):
    base, insert, _ = CHIPS[denom]
    m = core.Mat(f'chip_{denom}')
    tc, sep = object_coords(m)
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', x, x), m.math('MULTIPLY', y, y)))
    ang = m.math('ARCTAN2', y, x)
    # Six inserts around the edge, wrapping 3 mm into both faces.
    phase = m.math('FRACT', m.math('MULTIPLY', m.math('ADD', ang, math.pi), 6.0 / (2 * math.pi)))
    in_insert = m.math('MULTIPLY', smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', phase, 0.5)), 0.11, 0.004), ramp(m, r, RING_R - 0.0002, RING_R + 0.0002))
    color = m.mix(in_insert, core.hex_linear(base), core.hex_linear(insert))
    # The molded ring: a fine groove, and the clay a touch lighter where the mold polished it.
    groove = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', r, 0.0145)), 0.00012, 0.00008)
    face = m.math('MAXIMUM', m.math('GREATER_THAN', z, T - 0.0002), m.math('LESS_THAN', z, 0.0002))
    # The printed inlay on both faces (mirrored underneath so it reads the right way from below).
    top_uv = planar(m, sep, 'X', 'Y', -INLAY_R, INLAY_R, -INLAY_R, INLAY_R)
    bottom_uv = planar(m, sep, 'X', 'Y', -INLAY_R, INLAY_R, -INLAY_R, INLAY_R, mirror_a=True)
    uv = m.node('ShaderNodeMix')
    uv.data_type = 'VECTOR'
    m.link(m.math('GREATER_THAN', z, T / 2), uv.inputs['Factor'])
    m.link(bottom_uv, uv.inputs[4])
    m.link(top_uv, uv.inputs[5])
    ink, alpha, _, ink_rough = image_surface(m, inlay, uv.outputs[1], extension='CLIP')  # the vector result
    inlay_mask = m.math('MULTIPLY', m.math('MULTIPLY', alpha, face), smooth_less(m, r, INLAY_R, 0.00005))
    color = m.mix(inlay_mask, color, ink)
    # Handling: grime darkens the edges and the ring; the inlay's print is worn at its rim.
    grime = m.math('MULTIPLY', ramp(m, r, 0.0175, R), ramp(m, noise(m, tc.outputs['Object'], scale=900.0, detail=3.0), 0.4, 0.8))
    color = m.mix(m.math('MULTIPLY', grime, 0.35), color, core.hex_linear(0x3a332b))
    color = m.mix(m.math('MULTIPLY', groove, 0.5), color, core.hex_linear(0x000000))
    clay = noise(m, tc.outputs['Object'], scale=3000.0, detail=2.0)
    rough = m.math('ADD', 0.52, m.math('MULTIPLY', clay, 0.08))
    rough = mix_float(m, inlay_mask, rough, ink_rough)
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', 0.0)
    height = m.math('SUBTRACT', m.math('MULTIPLY', clay, 0.4), groove)
    bump = m.node('ShaderNodeBump', Strength=0.12, Distance=0.00004)
    m.link(height, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


# ------------------------------------------------------------------ the Embercrest's tournament set

# Value printed; body; edge spot; the stripe through each spot; the inlay's ground, its print, its rim; the crest in one
# ink on a light inlay, in full colour on the 25,000's navy one.
T_CHIPS = {
    'T100': dict(value='100', body=0x19191c, spot=0xebe7df, stripe=0xd9531e, inlay=0xf0ece3, ink=0x19191c, rim=0x19191c),
    'T500': dict(value='500', body=0x5b2c86, spot=0xebe7df, stripe=0xf6b04a, inlay=0xf0ece3, ink=0x4a2370, rim=0x5b2c86),
    'T1K': dict(value='1,000', body=0xf1c232, spot=0x121c33, stripe=0xebe7df, inlay=0xf0ece3, ink=0x121c33, rim=0xd9a51c),
    'T5K': dict(value='5,000', body=0xe8721f, spot=0xebe7df, stripe=0x121c33, inlay=0xf0ece3, ink=0xb4500f, rim=0xe8721f),
    'T25K': dict(value='25,000', body=0xe9e5dc, spot=0xd9531e, stripe=0xf6b04a, inlay=0x121c33, ink=0xebe7df, rim=0xd9531e),
}
T_INLAY_R = 0.013
T_SPOTS = 6
# A low lathe (radius, z): flat faces from the axis, a rolled edge. The inlay's lip is in the normal map.
T_PROFILE = [(0.0, 0.0), (R - 0.0012, 0.0), (R - 0.0003, 0.0003), (R, 0.0009), (R, T - 0.0009), (R - 0.0003, T - 0.0003), (R - 0.0012, T), (0.0, T)]
# SM_ChipStacks_EC: a Nightly's 20,000 and a Big Stack's two 5,000s (denomination, count, x, y), big chips in front (-Y).
EC_STACKS = [('T1K', 15, -0.0207, -0.0115), ('T5K', 2, 0.0207, -0.0115), ('T500', 8, -0.0207, 0.0295), ('T100', 10, 0.0207, 0.0295)]


def t_inlay_sheet(key):
    """A tournament chip's inlay (millimetres, 26 across, centred): its rim in the chip's colour, EMBERCREST over the
    top, the crest, and the value under it."""
    import sponsors
    c = T_CHIPS[key]
    d = T_INLAY_R * 2000
    s = Sheet(d, d)
    m = d / 2
    s.circle(m, m, m, c['inlay'], n=160, rough=0.35)
    s.ring(m, m, m - 0.8, m, c['rim'], rough=0.35)
    s.ring(m, m, m - 1.45, m - 1.25, c['rim'], rough=0.35)
    s.text_on_arc('EMBERCREST', m, m, 9.1, 1.75, c['ink'], face='Black', tracking=1.3, rough=0.4)
    if c['inlay'] == 0x121c33:
        brand.mark(s, m, m + 2.4, 10.2, ground=c['inlay'])
    else:
        brand.mark(s, m, m + 2.4, 10.2, mono=0x121c33, ground=c['inlay'], mono_ember=brand.EMBER)
    size = min(6.2, 14.0 / max(sponsors.text_width(s, c['value'], 1.0, 'Black'), 1e-6))
    s.text(c['value'], m, m - 8.0, size, c['ink'], face='Black', align='CENTER', rough=0.35)
    return s.render(f'chip_{key}_inlay', 512)


def t_chip_material(key, inlay):
    c = T_CHIPS[key]
    m = core.Mat(f'chip_{key}')
    tc, sep = object_coords(m)
    x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', x, x), m.math('MULTIPLY', y, y)))
    ang = m.math('ARCTAN2', y, x)
    # Six spots round the edge, each with a stripe down its middle, wrapping 3 mm into both faces.
    phase = m.math('FRACT', m.math('MULTIPLY', m.math('ADD', ang, math.pi), T_SPOTS / (2 * math.pi)))
    off = m.math('ABSOLUTE', m.math('SUBTRACT', phase, 0.5))
    on_ring = ramp(m, r, RING_R - 0.0002, RING_R + 0.0002)
    spot = m.math('MULTIPLY', smooth_less(m, off, 0.165, 0.004), on_ring)
    stripe = m.math('MULTIPLY', smooth_less(m, off, 0.03, 0.003), on_ring)
    color = m.mix(spot, core.hex_linear(c['body']), core.hex_linear(c['spot']))
    color = m.mix(stripe, color, core.hex_linear(c['stripe']))
    # The mould: a fine groove ring, and ridges radiating across the clay between the inlay and the spots.
    groove = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', r, 0.0146)), 0.00012, 0.00008)
    band = m.math('MULTIPLY', ramp(m, r, 0.0134, 0.0138), m.math('SUBTRACT', 1.0, ramp(m, r, 0.0157, 0.0161)))
    ridges = m.math('MULTIPLY', m.math('ADD', 0.5, m.math('MULTIPLY', m.math('SINE', m.math('MULTIPLY', ang, 180.0)), 0.5)), band)
    face = m.math('MAXIMUM', m.math('GREATER_THAN', z, T - 0.0002), m.math('LESS_THAN', z, 0.0002))
    # The inlay on both faces (mirrored underneath so it reads the right way from below).
    top_uv = planar(m, sep, 'X', 'Y', -T_INLAY_R, T_INLAY_R, -T_INLAY_R, T_INLAY_R)
    bottom_uv = planar(m, sep, 'X', 'Y', -T_INLAY_R, T_INLAY_R, -T_INLAY_R, T_INLAY_R, mirror_a=True)
    uv = m.node('ShaderNodeMix')
    uv.data_type = 'VECTOR'
    m.link(m.math('GREATER_THAN', z, T / 2), uv.inputs['Factor'])
    m.link(bottom_uv, uv.inputs[4])
    m.link(top_uv, uv.inputs[5])
    ink, alpha, _, ink_rough = image_surface(m, inlay, uv.outputs[1], extension='CLIP')
    in_inlay = smooth_less(m, r, T_INLAY_R, 0.00005)
    inlay_mask = m.math('MULTIPLY', m.math('MULTIPLY', alpha, face), in_inlay)
    color = m.mix(inlay_mask, color, ink)
    # Handling, light (the room's chips are kept clean): grime on the edge, the groove darker.
    grime = m.math('MULTIPLY', ramp(m, r, 0.0175, R), ramp(m, noise(m, tc.outputs['Object'], scale=900.0, detail=3.0), 0.4, 0.8))
    color = m.mix(m.math('MULTIPLY', grime, 0.12), color, core.hex_linear(0x3a332b))
    color = m.mix(m.math('MULTIPLY', groove, 0.45), color, core.hex_linear(0x000000))
    clay = noise(m, tc.outputs['Object'], scale=3000.0, detail=2.0)
    rough = m.math('ADD', 0.5, m.math('MULTIPLY', clay, 0.08))
    rough = mix_float(m, inlay_mask, rough, ink_rough)
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', 0.0)
    # Height: the clay's grain and ridges, the groove, and the clay standing 0.1 mm proud of the inlay.
    lip = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, in_inlay), face)
    height = m.math('ADD', m.math('SUBTRACT', m.math('MULTIPLY', clay, 0.4), groove), m.math('ADD', m.math('MULTIPLY', ridges, 0.3), m.math('MULTIPLY', lip, 1.6)))
    bump = m.node('ShaderNodeBump', Strength=0.14, Distance=0.00004)
    m.link(height, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def t_chip(key, inlay):
    chip = core.lathe(f'chip_{key}', T_PROFILE, segments=32)
    core.orient_normals(chip)
    core.assign(chip, t_chip_material(key, inlay))
    core.finish_hard_surface(chip, 40)
    core.uv_layout(chip, [
        (lambda f: f.normal.z > 0.5, 'planar', ('x', 'y', -R, R, -R, R), (0.0, 0.5, 0.5, 1.0)),
        (lambda f: f.normal.z < -0.5, 'planar', ('-x', 'y', -R, R, -R, R), (0.5, 0.5, 1.0, 1.0)),
        (lambda f: True, 'cylinder', (0.0, T), (0.0, 0.0, 1.0, 0.12)),
    ])
    return chip


def build():
    core.reset()
    inlays = {d: inlay_sheet(d) for d in CHIPS}
    t_inlays = {t: t_inlay_sheet(t) for t in TOURNAMENT}
    core.reset()
    chips = []
    for denom in CHIPS:
        chip = core.lathe(f'chip_{denom}', PROFILE, segments=48)
        core.orient_normals(chip)
        core.assign(chip, chip_material(denom, inlays[denom]))
        core.finish_hard_surface(chip, 40)
        core.uv_layout(chip, [
            (lambda f: f.normal.z > 0.5, 'planar', ('x', 'y', -R, R, -R, R), (0.0, 0.5, 0.5, 1.0)),
            (lambda f: f.normal.z < -0.5, 'planar', ('-x', 'y', -R, R, -R, R), (0.5, 0.5, 1.0, 1.0)),
            (lambda f: True, 'cylinder', (0.0, T), (0.0, 0.0, 1.0, 0.12)),
        ])
        chips.append(chip)
    for key in TOURNAMENT:
        chips.append(t_chip(key, t_inlays[key]))
    return chips


def _stacked(name, spec, by_denom, rnd):
    """Copies of the baked chips in columns (denomination, count, x, y), the faces they hide removed, joined as name."""
    parts = []
    for denom, count, sx, sy in spec:
        for i in range(count):
            src = by_denom[denom]
            dup = src.copy()
            dup.data = src.data.copy()
            core.link(dup)
            bm = bmesh.new()
            bm.from_mesh(dup.data)
            bm.normal_update()
            top = i == count - 1
            # Only the covered middle of a face goes: the rounded rims leave a groove between
            # stacked chips where the outer ring still shows.
            covered = lambda f: f.calc_center_median().xy.length < R - 0.004  # noqa: E731
            doomed = [f for f in bm.faces if covered(f) and (f.normal.z < -0.5 or (not top and f.normal.z > 0.5))]
            bmesh.ops.delete(bm, geom=doomed, context='FACES_ONLY')
            spin = Matrix.Rotation(rnd.uniform(0, 2 * math.pi), 4, 'Z')
            bm.transform(Matrix.Translation(Vector((sx + rnd.uniform(-0.0005, 0.0005), sy + rnd.uniform(-0.0005, 0.0005), i * (T + 0.00002)))) @ spin)
            bm.to_mesh(dup.data)
            bm.free()
            parts.append(dup)
    return core.join(parts, name)


def assemble(chips):
    """Copies the baked chips into the stacks; hidden faces (under each chip, between chips) go."""
    by_denom = {c.name.split('_', 1)[1]: c for c in chips}
    stacks = _stacked('SM_ChipStacks', STACKS, by_denom, random.Random(11))
    stacks_ec = _stacked('SM_ChipStacks_EC', EC_STACKS, by_denom, random.Random(23))
    # Each baked chip also ships on its own (origin at the bottom center) for the back room's stacks and bets.
    for c in chips:
        denom = c.name.split('_', 1)[1]
        c.name = f'SM_Chip_{denom}'
        c.data.name = c.name
    return [stacks, stacks_ec] + chips


REVIEW_VIEWS = [('front', -20, 22, 2.2, (0.0, 0.0, 0.015)), ('top', 0, 62, 2.7, (0.084, -0.05, 0.0)), ('detail', -28, 22, 0.55, (0.06, 0.02, 0.02))]


def pose_for_review(objs):
    """Dee's chips at the left as they were; the Embercrest's set at the right, its singles in a row in front of its
    stacks."""
    for o in objs:
        name = o.name
        if name == 'SM_ChipStacks':
            o.location = (-0.12, 0.03, 0.0)
        elif name == 'SM_ChipStacks_EC':
            o.location = (0.06, 0.03, 0.0)
        elif name.startswith('SM_Chip_T'):
            o.location = (TOURNAMENT.index(name[8:]) * 0.042, -0.05, 0.0)
        elif name.startswith('SM_Chip_') and name[8:] in CHIPS:
            o.location = (-0.19 + list(CHIPS).index(name[8:]) * 0.042, -0.05, 0.0)
