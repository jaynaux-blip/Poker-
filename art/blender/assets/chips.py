"""SM_ChipStacks: a few stacks of clay chips from the Tuesday game at the laundromat.

Four denominations of 39 mm, 10 g-style clay chips:
- edge inserts that wrap into the face;
- a molded ring;
- a printed inlay from the "Spin Cycle Club".

Each denomination is baked once, then copied into the stacks with a little jitter (assemble), so
every chip is crisp and the texture budget stays at four small sets. Faces hidden inside a stack
are removed.

Coordinates (meters): origin on the desk at the middle of the group, Z up; the group faces -Y.
SM_Chip_<denomination> are the single chips, origin at the bottom center.
"""
import math
import random

import bpy  # noqa: I001 (bpy first: the PyPI build registers bmesh and mathutils on import)
import bmesh
from mathutils import Matrix, Vector

from artkit import core
from artkit.shade import image_surface, mix_float, noise, object_coords, planar, ramp, smooth_less
from artkit.sheet import Sheet

MESHES = ['SM_ChipStacks', 'SM_Chip_1', 'SM_Chip_5', 'SM_Chip_25', 'SM_Chip_100']
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

REVIEW_VIEWS = [('front', -28, 24, 2.3), ('top', 10, 64, 2.1), ('detail', -35, 30, 0.9, (0.0, 0.0, 0.03))]

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


def build():
    core.reset()
    inlays = {d: inlay_sheet(d) for d in CHIPS}
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
    return chips


def assemble(chips):
    """Copies the baked chips into the stacks; hidden faces (under each chip, between chips) go."""
    by_denom = {c.name.split('_', 1)[1]: c for c in chips}
    rnd = random.Random(11)
    parts = []
    for denom, count, sx, sy in STACKS:
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
    stacks = core.join(parts, 'SM_ChipStacks')
    # Each baked chip also ships on its own (origin at the bottom center) for the back room's stacks and bets.
    for c in chips:
        denom = c.name.split('_', 1)[1]
        c.name = f'SM_Chip_{denom}'
        c.data.name = c.name
    return [stacks] + chips


def pose_for_review(objs):
    """The single chips stand in a row in front of the stacks."""
    singles = [o for o in objs if o.name.startswith('SM_Chip_')]
    for i, o in enumerate(singles):
        o.location = (-0.06 + i * 0.042, -0.07, 0.0)
