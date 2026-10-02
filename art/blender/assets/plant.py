"""SM_Plant: GearDrop's desk plant, a golden pothos in a glazed pot on the windowsill.

A white glazed pot on its saucer, the soil, and nine vines: three standing up, six trailing over the rim,
along the sill to either side and over the sill's front edge, with heart-shaped leaves (folded along the
midrib, variegated) that shrink toward the tips.

Coordinates (meters), front toward -Y, Z up: origin on the sill under the pot; the sill's front edge is
SILL_EDGE in front of it (toward -Y) and the window glass about 0.11 behind.
"""
import math
import random

import bpy  # noqa: F401,I001
import bmesh
from mathutils import Matrix, Vector

from artkit import core, parts
from artkit.shade import noise, object_coords, ramp, scaled

MESHES = ['SM_Plant']
DOUBLE_SIDED = True  # leaves are single sheets
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'plant_leaf': 2048, 'plant_soil': 256, 'plant_vine': 256}
AO_DISTANCE = 0.02
REVIEW_VIEWS = [('front', -20, 18, 0.9), ('side', 70, 25, 0.9), ('detail', -10, 30, 0.4, (0.0, -0.05, 0.1))]

SILL_EDGE = 0.09
POT_H = 0.10
SOIL_Z = 0.086
LEAF_EDGE = [(0.18, -0.08), (0.36, -0.06), (0.48, 0.05), (0.52, 0.2), (0.48, 0.38), (0.38, 0.56), (0.25, 0.73), (0.12, 0.88)]
MIDRIB = [0.0, 0.06, 0.12, 0.2, 0.3, 0.42, 0.56, 0.72, 0.86, 1.0]


def glaze_material():
    m = core.Mat('plant_pot')
    tc, sep = object_coords(m)
    craze = ramp(m, noise(m, tc.outputs['Object'], scale=420.0, detail=6.0, distortion=1.5), 0.62, 0.64)
    pool = ramp(m, sep.outputs['Z'], 0.0, 0.02)
    m.set('Base Color', m.mix(m.math('MULTIPLY', craze, 0.25), m.mix(pool, core.hex_linear(0xc9c4ba), core.hex_linear(0xe8e4dc)), core.hex_linear(0x8f887d)))
    m.set('Roughness', m.math('ADD', 0.12, m.math('MULTIPLY', craze, 0.1)))
    m.set('Coat Weight', 0.3)
    return m


def leaf_material():
    m = core.Mat('plant_leaf')
    tc, sep = object_coords(m)
    streak = ramp(m, noise(m, scaled(m, sep, 140.0, 140.0, 40.0), detail=4.0, distortion=2.0), 0.58, 0.72)
    patch = noise(m, tc.outputs['Object'], scale=35.0, detail=2.0)
    green = m.mix(patch, core.hex_linear(0x2c5f22), core.hex_linear(0x4b8a2f))
    color = m.mix(m.math('MULTIPLY', streak, 0.85), green, core.hex_linear(0xd9cf73))
    m.set('Base Color', color)
    m.set('Roughness', m.math('ADD', 0.32, m.math('MULTIPLY', patch, 0.1)))
    m.set('Subsurface Weight', 0.15)
    m.set('Subsurface Radius', (0.4, 0.8, 0.2))
    m.set('Sheen Weight', 0.1)
    return m


def soil_material():
    m = core.Mat('plant_soil')
    tc, _ = object_coords(m)
    clumps = noise(m, tc.outputs['Object'], scale=600.0, detail=5.0, roughness=0.7)
    perlite = ramp(m, noise(m, tc.outputs['Object'], scale=900.0, detail=1.0), 0.72, 0.75)
    m.set('Base Color', m.mix(perlite, m.mix(clumps, core.hex_linear(0x1a120c), core.hex_linear(0x2e2117)), core.hex_linear(0xd8d4cc)))
    m.set('Roughness', 0.95)
    bump = m.node('ShaderNodeBump', Strength=0.8, Distance=0.002)
    m.link(clumps, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def leaf(size, fold, curl):
    """A heart-shaped leaf with its stem point at the origin, the blade along +Y, facing +Z; folded at the midrib."""
    bm = bmesh.new()

    def bend(x, y):
        return Vector((x * size, y * size, (abs(x) * fold - curl * (y - 0.45) ** 2) * size))

    mid = [bm.verts.new(bend(0.0, y)) for y in MIDRIB]
    for s in (-1, 1):
        edge = [bm.verts.new(bend(s * x, y)) for x, y in LEAF_EDGE]
        bm.faces.new((mid[0], mid[1], edge[0]) if s > 0 else (mid[1], mid[0], edge[0]))
        for k in range(len(edge) - 1):
            quad = (mid[k + 1], mid[k + 2], edge[k + 1], edge[k])
            bm.faces.new(quad if s > 0 else tuple(reversed(quad)))
        bm.faces.new((mid[-2], mid[-1], edge[-1]) if s > 0 else (mid[-1], mid[-2], edge[-1]))
    return core.mesh_object('leaf', bm)


def vine_path(rng, angle, kind):
    """Control points for a vine from the soil: 'up' stands, 'side' drapes onto the sill, 'front' hangs over its edge."""
    d = Vector((math.sin(angle), -math.cos(angle), 0.0))
    start = d * rng.uniform(0.0, 0.025) + Vector((0, 0, SOIL_Z))
    if kind == 'up':
        h = rng.uniform(0.09, 0.15)
        return [start, start + d * 0.01 + Vector((0, 0, h * 0.5)), start + d * rng.uniform(0.03, 0.06) + Vector((0, 0, h))]
    rim = d * 0.068 + Vector((0, 0, POT_H + 0.012))
    pts = [start, start + d * 0.02 + Vector((0, 0, 0.03)), rim, d * 0.085 + Vector((0, 0, POT_H - 0.02))]
    if kind == 'side':
        # Along the sill, mostly sideways (it is only SILL_EDGE deep in front of the pot).
        reach = rng.uniform(0.16, 0.3)
        a = d * 0.095
        s = Vector((math.copysign(1.0, d.x), d.y * 0.25, 0.0)).normalized()
        pts += [a + Vector((0, 0, 0.012)), a + s * reach * 0.5 + Vector((0, 0, 0.004)), a + s * reach + Vector((0, -0.01, 0.004))]
    else:
        # Out to the sill's front edge, then down past it.
        edge = Vector((d.x / max(-d.y, 0.2) * SILL_EDGE, -SILL_EDGE - 0.004, 0.0))
        drop = rng.uniform(0.05, 0.12)
        pts += [edge * 0.8 + Vector((0, 0, 0.012)), edge + Vector((0, -0.004, 0.0)), edge + Vector((0, -0.01, -drop * 0.5)), edge + Vector((0.0, -0.012, -drop))]
    return pts


def build():
    core.reset()
    rng = random.Random(7)
    pot_m, leaf_m, soil_m = glaze_material(), leaf_material(), soil_material()
    vine_m = parts.plastic('plant_vine', 0x557a2c, rough=0.5, scuffs=0.0)
    saucer_m = parts.gloss('plant_saucer', 0xb9b3a8, rough=0.2)
    out = []
    pot = core.lathe('pot', [(0.0, 0.004), (0.045, 0.004), (0.047, 0.007), (0.057, POT_H - 0.004), (0.06, POT_H), (0.0605, POT_H + 0.003), (0.058, POT_H + 0.004),
                             (0.056, POT_H), (0.047, 0.012), (0.0, 0.012)], segments=72)
    core.orient_normals(pot)
    core.assign(pot, pot_m)
    saucer = core.lathe('saucer', [(0.0, 0.0), (0.056, 0.0), (0.064, 0.012), (0.066, 0.013), (0.062, 0.0125), (0.054, 0.003), (0.0, 0.003)], segments=72)
    core.orient_normals(saucer)
    core.assign(saucer, saucer_m)
    soil = core.lathe('soil', [(0.0, SOIL_Z + 0.004), (0.03, SOIL_Z + 0.002), (0.0545, SOIL_Z), (0.054, SOIL_Z - 0.006), (0.0, SOIL_Z - 0.006)], segments=48)
    core.orient_normals(soil)
    core.assign(soil, soil_m)
    out += [pot, saucer, soil]

    plan = [(-20, 'up'), (35, 'up'), (150, 'up'), (-75, 'side'), (-100, 'side'), (80, 'side'), (105, 'side'), (-25, 'front'), (20, 'front')]
    for deg, kind in plan:
        pts = core.catmull_rom(vine_path(rng, math.radians(deg + rng.uniform(-8, 8)), kind), 8)
        vine = parts.tube('vine', pts, 0.0017, 6, 1)
        core.assign(vine, vine_m)
        out.append(vine)
        # Leaves at nodes along the vine, alternating sides, smaller toward the tip.
        lengths = [0.0]
        for a, b in zip(pts, pts[1:]):
            lengths.append(lengths[-1] + (Vector(b) - Vector(a)).length)
        total = lengths[-1]
        n = max(3, int(total / 0.045))
        for k in range(n):
            t = (k + 0.6) / n * total
            i = next(j for j in range(len(lengths) - 1) if lengths[j + 1] >= t)
            p = Vector(pts[i]).lerp(Vector(pts[i + 1]), (t - lengths[i]) / max(lengths[i + 1] - lengths[i], 1e-6))
            along = (Vector(pts[i + 1]) - Vector(pts[i])).normalized()
            size = (0.05 - 0.022 * k / max(n - 1, 1)) * rng.uniform(0.85, 1.12)
            lf = leaf(size, rng.uniform(0.18, 0.32), rng.uniform(0.25, 0.5))
            side = 1 if k % 2 else -1
            # The blade points out from the vine and toward the light (up and toward the room or the sides).
            out_dir = along.cross(Vector((0, 0, 1)))
            if out_dir.length < 1e-3:
                out_dir = Vector((1, 0, 0))
            out_dir = (out_dir.normalized() * side + along * 0.6 + Vector((0, 0, 0.5 if kind != 'front' else 0.1))).normalized()
            up = Vector((0, 0, 1)) - out_dir * out_dir.z
            up = (up + Vector((0, -0.4, 0))).normalized()
            x = out_dir.cross(up).normalized()
            z = x.cross(out_dir).normalized()
            m = Matrix((x, out_dir, z)).transposed().to_4x4()
            m = Matrix.Translation(p) @ m @ Matrix.Rotation(rng.uniform(-0.3, 0.3), 4, 'Y')
            # The leaf sits at the end of its petiole, a little out from the vine.
            base = p + out_dir * size * 0.3 + z * size * 0.06
            lf.data.transform(Matrix.Translation(base - p) @ m)
            petiole = parts.tube('petiole', [p, (p + base) / 2 + z * 0.003, base], 0.0009, 5, 3)
            core.assign(lf, leaf_m)
            core.assign(petiole, vine_m)
            out += [lf, petiole]
    return [parts.finish(out, 'SM_Plant', 60)]


def pose_for_review(objs):
    slab = parts.rbox('review_sill', (0.0, -0.01, -0.015), (0.8, 0.2, 0.03), 0.0)
    m = core.Mat('review_sill')
    m.set('Base Color', core.hex_linear(0x9a968c))
    m.set('Roughness', 0.5)
    core.assign(slab, m)
