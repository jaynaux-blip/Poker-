"""SM_TrashCan: the city's litter basket on the curb.

A slatted steel basket in municipal green: a solid band at the foot and the rim, vertical bars between,
a rain hood raised on four struts over the opening, three leveling feet. A black liner bag shows through
the bars and bunches over the rim under the hood. A stenciled plate on the rim: RIVERSIDE SANITATION.

Coordinates (meters), front toward -Y (the plate), Z up: origin on the sidewalk under the basket's axis.
"""
import math

import bpy  # noqa: F401,I001
from mathutils import Matrix, Vector

from artkit import core, parts, street
from artkit.shade import image_surface, object_coords, planar
from artkit.sheet import Sheet

MESHES = ['SM_TrashCan']
DOUBLE_SIDED = True
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'can_plate': 512, 'can_feet': 128}
AO_DISTANCE = 0.05
REVIEW_VIEWS = [('front', -25, 14, 2.5), ('top', 30, 55, 2.4), ('detail', -10, 8, 1.0, (0.0, -0.3, 0.84))]

R = 0.29
H = 0.98
FOOT = 0.03
BAND0 = (FOOT, FOOT + 0.11)
BAND1 = (0.80, 0.88)
PLATE = (0.26, 0.05)


def plate_sheet():
    w, h = PLATE[0] * 1000, PLATE[1] * 1000
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0xe9e4d6, rough=0.5)
    s.rect(3, 3, w - 6, h - 6, 0x1d5a3c, rough=0.5)
    s.text('RIVERSIDE', w / 2, h * 0.52, 15, 0xe9e4d6, face='Black', align='CENTER', tracking=1.25, rough=0.5)
    s.text('SANITATION  ·  KEEP FIFTH CLEAN', w / 2, h * 0.2, 6.5, 0xe9e4d6, face='Bold', align='CENTER', tracking=1.3, rough=0.5)
    return s.render('can_plate', 1024)


def plate_material(printed, half_w, z0, z1):
    m = core.Mat('can_plate')
    tc, sep = object_coords(m)
    uv = planar(m, sep, 'X', 'Z', -half_w, half_w, z0, z1)
    ink, alpha, metal, rough = image_surface(m, printed, uv)
    m.set('Base Color', ink)
    m.set('Roughness', rough)
    return m


def bag_material():
    """A black bin liner: glossy plastic, stretched thin and grey where it's pulled over the rim."""
    m = core.Mat('can_bag')
    tc, sep = object_coords(m)
    from artkit.shade import noise
    crinkle = noise(m, tc.outputs['Object'], scale=35.0, detail=6.0, distortion=1.2)
    m.set('Base Color', m.mix(m.math('MULTIPLY', crinkle, 0.5), core.hex_linear(0x0b0b0c), core.hex_linear(0x2a2b2e)))
    m.set('Roughness', m.math('ADD', 0.22, m.math('MULTIPLY', crinkle, 0.2)))
    bump = m.node('ShaderNodeBump', Strength=0.6, Distance=0.004)
    m.link(crinkle, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def curved_panel(name, r0, r1, a0, a1, z0, z1, n=24):
    """A curved plate: the ring sector between radii r0 and r1, angles a0..a1, z0..z1 (a closed solid)."""
    import bmesh
    bm = bmesh.new()
    rings = []
    for i in range(n + 1):
        a = a0 + (a1 - a0) * i / n
        c, s_ = math.cos(a), math.sin(a)
        rings.append([bm.verts.new((r * c, r * s_, z)) for r, z in ((r1, z0), (r1, z1), (r0, z1), (r0, z0))])
    for i in range(n):
        for k in range(4):
            bm.faces.new((rings[i][k], rings[i + 1][k], rings[i + 1][(k + 1) % 4], rings[i][(k + 1) % 4]))
    bm.faces.new(rings[0][::-1])
    bm.faces.new(rings[n])
    obj = core.mesh_object(name, bm)
    core.orient_normals(obj)
    return obj


def build():
    core.reset()
    steel = street.painted('can_steel', 0x1f4f38, rough=0.5, chips=0.6, rust=0.5, grime=0.7, grime_height=0.4, streaks=0.6)
    bag_m = bag_material()
    feet_m = street.painted('can_feet', 0x2a2a2a, rough=0.6, chips=1.0, rust=1.0, grime=1.0)
    out = []

    def band(z0, z1, r_out, t=0.006):
        # A shell ring with a rolled lip top and bottom (an outer and an inner wall).
        prof = [(r_out - t, z0), (r_out, z0), (r_out + 0.004, z0 + 0.008), (r_out, z0 + 0.016), (r_out, z1 - 0.016),
                (r_out + 0.004, z1 - 0.008), (r_out, z1), (r_out - t, z1), (r_out - t, z0)]
        return parts.lathe_at('band', prof, Matrix(), segments=72)

    out.append((band(*BAND0, R), steel))
    out.append((band(*BAND1, R), steel))
    # The floor of the basket.
    out.append((parts.disc('floor', (0, 0, BAND0[0] + 0.01), (0, 0, 1), R - 0.006, 0.008, 72), steel))
    # Bars between the bands, flat bar standing edge-out.
    bars = 40
    for k in range(bars):
        a = 2 * math.pi * k / bars
        c = Vector((R * math.cos(a), R * math.sin(a), (BAND0[1] + BAND1[0]) / 2))
        bar = parts.rbox('bar', (0, 0, 0), (0.008, 0.026, BAND1[0] - BAND0[1] + 0.02), 0.002)
        bar.data.transform(Matrix.Translation(c) @ Matrix.Rotation(a + math.pi / 2, 4, 'Z'))
        out.append((bar, steel))
    # Feet.
    for k in range(3):
        a = 2 * math.pi * k / 3 + math.pi / 2
        c = Vector(((R - 0.05) * math.cos(a), (R - 0.05) * math.sin(a), FOOT / 2))
        out.append((parts.disc('foot', c, (0, 0, 1), 0.03, FOOT, 16), feet_m))
    # The rain hood on four struts, a gap all round to drop things in.
    hood = [(0.0, 1.0), (0.12, 0.99), (0.22, 0.968), (0.285, 0.945), (R + 0.025, 0.925), (R + 0.03, 0.915),
            (R + 0.022, 0.912), (0.27, 0.93), (0.2, 0.952), (0.1, 0.97), (0.0, 0.975)]
    out.append((parts.lathe_at('hood', hood, Matrix(), segments=72), steel))
    for k in range(4):
        a = 2 * math.pi * (k + 0.5) / 4
        p0 = Vector(((R - 0.004) * math.cos(a), (R - 0.004) * math.sin(a), BAND1[1] - 0.01))
        p1 = Vector(((R - 0.01) * math.cos(a), (R - 0.01) * math.sin(a), 0.935))
        out.append((parts.rbox('strut', (p0 + p1) / 2, (0.03, 0.006, (p1 - p0).length + 0.01), 0.002, rot=Matrix.Rotation(a + math.pi / 2, 3, 'Z')), steel))
    # The liner: inside the bars, bunched over the rim and tucked under.
    rng = [0.0, 0.017, -0.012, 0.022, -0.006, 0.014, -0.019, 0.009]
    liner = [(0.0, BAND0[0] + 0.02), (R - 0.03, BAND0[0] + 0.02), (R - 0.018, BAND0[1]), (R - 0.016, BAND1[0] - 0.05),
             (R - 0.014, BAND1[1] - 0.005), (R - 0.004, BAND1[1] + 0.022), (R + 0.011, BAND1[1] + 0.01), (R + 0.012, BAND1[1] - 0.004),
             (R + 0.006, BAND1[1] - 0.012)]
    bag = parts.lathe_at('liner', liner, Matrix(), segments=96)
    # Folds: push the bag in and out around the rim.
    for v in bag.data.vertices:
        a = math.atan2(v.co.y, v.co.x)
        if v.co.z > BAND1[0] - 0.06 and v.co.xy.length > 0.05:
            f = sum(rng[i] * math.sin((i + 3) * a + i) for i in range(len(rng))) * 0.6
            v.co.x += math.cos(a) * f * 0.5
            v.co.y += math.sin(a) * f * 0.5
            v.co.z += f * 0.4
    out.append((bag, bag_m))
    # The plate on the rim band, facing the sidewalk.
    printed = plate_sheet()
    zc = (BAND1[0] + BAND1[1]) / 2 - 0.008
    half = PLATE[0] / 2 / (R + 0.004)
    plate = curved_panel('plate', R + 0.001, R + 0.004, -math.pi / 2 - half, -math.pi / 2 + half, zc - PLATE[1] / 2, zc + PLATE[1] / 2)
    out.append((plate, plate_material(printed, (R + 0.004) * math.sin(half), zc - PLATE[1] / 2, zc + PLATE[1] / 2)))
    for o, m in out:
        core.assign(o, m)
    return [parts.finish([o for o, _ in out], 'SM_TrashCan', 40)]
