"""SM_DeskLamp: a balanced-arm desk lamp in black enamel, switched off and aimed at the desk.

- A weighted round base with a yoke.
- Twin-rod arms balanced by springs, with knobbed joints.
- A bell shade: black outside, white inside, with a frosted bulb.
- A cloth-covered cable that runs back and drops off the desk's rear edge.

Coordinates (meters): origin at the center of the base on the desk, Z up. The desk's back edge is
REAR_EDGE behind it (+Y), as the stage places the lamp.
"""
import math

import bpy  # noqa: I001 (bpy first: the PyPI build registers bmesh and mathutils on import)
import bmesh
from mathutils import Matrix, Vector

from artkit import core
from artkit.shade import noise, object_coords, powder_coat, ramp

MESHES = ['SM_DeskLamp']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'lamp_paint': 2048, 'lamp_bulb': 256, 'lamp_cable': 512}
AO_DISTANCE = 0.02

REAR_EDGE = 0.11
YAW = math.radians(-10)
P0 = Vector((0.0, 0.0, 0.062))                      # base pivot
P1 = Vector((-0.109, 0.0, 0.360))                   # elbow
P2 = P1 + 0.29 * Vector((math.cos(math.radians(12)), 0.0, math.sin(math.radians(12))))  # shade pivot
SHADE_DIR = Vector((0.42, 0.0, -1.0)).normalized()  # where the shade points (in the arm plane)
ROD_R = 0.0035
ROD_GAP = 0.011

REVIEW_VIEWS = [('front', -30, 14, 2.4), ('top', 25, 55, 2.3), ('detail', -40, 20, 0.7, (-0.05, 0.0, 0.12))]


# ------------------------------------------------------------------ materials

def paint_material():
    m = core.Mat('lamp_paint')
    tc, sep = object_coords(m)
    color, rough, metal, bump = powder_coat(m, tc, 0x151618, rough=0.42, chips=0.7)
    # Enamel over steel: a satin sheen, with fine dust settled on the upward faces.
    geo = m.node('ShaderNodeNewGeometry')
    up = m.node('ShaderNodeSeparateXYZ')
    m.link(geo.outputs['Normal'], up.inputs['Vector'])
    dust = m.math('MULTIPLY', ramp(m, up.outputs['Z'], 0.6, 0.97), ramp(m, noise(m, tc.outputs['Object'], scale=700.0, detail=2.0), 0.35, 0.75))
    color = m.mix(m.math('MULTIPLY', dust, 0.035), color, core.hex_linear(0x77736c))
    rough = m.math('ADD', rough, m.math('MULTIPLY', dust, 0.08))
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    m.set('Normal', bump.outputs['Normal'])
    return m


def simple(name, hex_color, rough, metal=0.0):
    m = core.Mat(name)
    m.set('Base Color', core.hex_linear(hex_color))
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    return m


def steel_material():
    m = core.Mat('lamp_steel')
    tc, _ = object_coords(m)
    streak = noise(m, tc.outputs['Object'], scale=250.0, detail=3.0)
    m.set('Base Color', core.hex_linear(0xb9bcc1))
    m.set('Metallic', 1.0)
    m.set('Roughness', m.math('ADD', 0.18, m.math('MULTIPLY', streak, 0.12)))
    return m


def cable_material():
    m = core.Mat('lamp_cable')
    tc, sep = object_coords(m)
    # Braided cloth: a fine twill along the cable.
    weave = noise(m, tc.outputs['Object'], scale=2200.0, detail=1.0)
    m.set('Base Color', m.mix(m.math('MULTIPLY', weave, 0.4), core.hex_linear(0x141414), core.hex_linear(0x2a2a2a)))
    m.set('Roughness', 0.8)
    bump = m.node('ShaderNodeBump', Strength=0.3, Distance=0.0002)
    m.link(weave, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


# ------------------------------------------------------------------ geometry

def rod(name, a, b, r, n=16):
    return core.sweep(name, [a, b], [(r * math.cos(2 * math.pi * k / n), r * math.sin(2 * math.pi * k / n)) for k in range(n)])


def spring(name, a, b, coil_r=0.0055, wire_r=0.0008, pitch=0.0021):
    """A tension spring from a to b: a tight helix with hooked ends drawn as short straight leads."""
    axis = (b - a)
    length = axis.length
    d = axis.normalized()
    ref = Vector((0, 0, 1)) if abs(d.z) < 0.9 else Vector((1, 0, 0))
    u = d.cross(ref).normalized()
    v = d.cross(u)
    turns = (length - 0.012) / pitch
    steps = int(turns * 10)
    pts = [a]
    for i in range(steps + 1):
        t = i / steps
        ang = t * turns * 2 * math.pi
        pts.append(a + d * (0.006 + t * (length - 0.012)) + u * (coil_r * math.cos(ang)) + v * (coil_r * math.sin(ang)))
    pts.append(b)
    return core.sweep(name, pts, [(wire_r * math.cos(2 * math.pi * k / 6), wire_r * math.sin(2 * math.pi * k / 6)) for k in range(6)])


def disc(name, center, axis, r, width, n=32):
    """A short cylinder (knob, axle, knuckle) centered at center along axis."""
    d = Vector(axis).normalized()
    return rod(name, center - d * width / 2, center + d * width / 2, r, n)


def arm(a, b, parts):
    """Two parallel painted rods from joint a to joint b, either side of the arm plane."""
    for s in (-1, 1):
        off = Vector((0.0, s * ROD_GAP, 0.0))
        parts.append(rod('rod', a + off, b + off, ROD_R))
    # Spacers tie the rods together near each end.
    for t in (0.08, 0.92):
        parts.append(disc('spacer', a + (b - a) * t, (0, 1, 0), 0.0028, 2 * ROD_GAP))


def shade_parts(paint, inside, bulb_mat, steel):
    """The bell shade, bulb and socket, built around -Z from the neck at the origin."""
    outer = [(0.0, 0.014), (0.010, 0.014), (0.018, 0.009), (0.021, 0.002), (0.024, -0.018), (0.032, -0.046), (0.045, -0.085),
             (0.059, -0.121), (0.068, -0.141), (0.0715, -0.1475), (0.0712, -0.1505), (0.0690, -0.1500), (0.0665, -0.1455),
             (0.0575, -0.1225), (0.0435, -0.0870), (0.0305, -0.0480), (0.0222, -0.0190), (0.0195, -0.004), (0.0, -0.004)]
    shade = core.lathe('shade', outer, segments=72)
    core.orient_normals(shade)
    shade.data.materials.append(paint.m)
    shade.data.materials.append(inside.m)
    bm = bmesh.new()
    bm.from_mesh(shade.data)
    bm.normal_update()
    for f in bm.faces:
        c = f.calc_center_median()
        radial = Vector((c.x, c.y, 0.0))
        f.material_index = 1 if (radial.length > 1e-6 and f.normal.dot(radial) < -0.2 and c.z < -0.006) else 0
    bm.to_mesh(shade.data)
    bm.free()
    socket = core.lathe('socket', [(0.0, -0.004), (0.0135, -0.004), (0.0135, -0.036), (0.0115, -0.038), (0.0, -0.038)], segments=32)
    core.orient_normals(socket)
    core.assign(socket, steel)
    glass = [(0.0, -0.036), (0.0120, -0.036), (0.0135, -0.048)] + [
        (0.030 * math.sin(math.radians(a)), -0.083 - 0.030 * math.cos(math.radians(a))) for a in range(140, -1, -10)]
    glass[-1] = (0.0, glass[-1][1])
    bulb = core.lathe('bulb', glass, segments=48)
    core.orient_normals(bulb)
    core.assign(bulb, bulb_mat)
    return [shade, socket, bulb]


def build():
    core.reset()
    paint, inside, steel = paint_material(), simple('lamp_inside', 0xe9e6de, 0.55), steel_material()
    bulb_mat, cable_mat = simple('lamp_bulb', 0xf3f1ea, 0.42), cable_material()
    painted, steel_parts = [], []

    # Weighted base: a stepped, rounded drum.
    base = core.lathe('base', [(0.0, 0.0), (0.074, 0.0), (0.077, 0.003), (0.077, 0.015), (0.074, 0.019), (0.052, 0.022),
                               (0.030, 0.026), (0.022, 0.030), (0.0, 0.030)], segments=96)
    core.orient_normals(base)
    painted.append(base)
    # Yoke plates either side of the base pivot, and the pivot axle with its tension knob.
    for s in (-1, 1):
        painted.append(core.mesh_object('yoke', core.slab(core.rounded_rect(-0.014, s * 0.016 - 0.002, 0.014, s * 0.016 + 0.002, 0.0015, steps=3),
                                                          0.026, P0.z + 0.012)))
    steel_parts.append(disc('axle', P0, (0, 1, 0), 0.0045, 0.042))
    painted.append(disc('knob', P0 + Vector((0, 0.026, 0)), (0, 1, 0), 0.010, 0.010))

    arm(P0, P1, painted)
    arm(P1, P2, painted)
    # Elbow knuckle and knob; the shade's knuckle.
    painted.append(disc('elbow', P1, (0, 1, 0), 0.0115, 2 * ROD_GAP + 0.008))
    painted.append(disc('elbow_knob', P1 + Vector((0, ROD_GAP + 0.011, 0)), (0, 1, 0), 0.0085, 0.008))
    painted.append(disc('shade_knuckle', P2, (0, 1, 0), 0.009, 2 * ROD_GAP + 0.006))
    # Balance springs: one each side of the lower arm, outside the rods, and one along the upper arm.
    lower = (P1 - P0).normalized()
    back = Vector((-lower.z, 0.0, lower.x))  # in the arm plane, behind the lower arm
    for s in (-1, 1):
        side = Vector((0, s * (ROD_GAP + 0.0085), 0))
        a = P0 + lower * 0.035 + back * 0.012 + side
        steel_parts.append(spring('spring', a, a + lower * 0.11, coil_r=0.0048))
        steel_parts.append(rod('link', a + lower * 0.11, P0 + lower * 0.2 + side * 0.72, 0.0009, 6))
        steel_parts.append(rod('anchor', a, P0 + side * 0.72 - lower * 0.004 + back * 0.012, 0.0009, 6))
    upper = (P2 - P1).normalized()
    ua = P1 + upper * 0.03 + Vector((0, 0, 0.012))
    steel_parts.append(spring('spring_upper', ua, ua + upper * 0.09, coil_r=0.0045))

    # The shade hangs from its knuckle, turned to point down at the desk.
    shade_objs = shade_parts(paint, inside, bulb_mat, steel)
    turn = Vector((0.0, 0.0, -1.0)).rotation_difference(SHADE_DIR).to_matrix().to_4x4()
    place = Matrix.Translation(P2 + SHADE_DIR * 0.012) @ turn
    for o in shade_objs:
        o.data.transform(place)
    neck = rod('neck', P2, P2 + SHADE_DIR * 0.016, 0.006, 20)
    painted.append(neck)

    paint_obj = core.join(painted, 'painted')
    core.assign(paint_obj, paint)
    steel_obj = core.join(steel_parts, 'steel')
    core.assign(steel_obj, steel)
    lamp = core.join([paint_obj, steel_obj] + shade_objs, 'lamp')
    lamp.data.transform(Matrix.Rotation(YAW, 4, 'Z'))

    # The cable leaves the back of the base, runs to the desk's rear edge and drops over it.
    ctrl = [(-0.020, 0.074, 0.010), (-0.026, 0.086, 0.004), (-0.030, 0.100, 0.0028), (-0.028, REAR_EDGE - 0.004, 0.0028),
            (-0.026, REAR_EDGE + 0.006, -0.004), (-0.025, REAR_EDGE + 0.010, -0.03), (-0.024, REAR_EDGE + 0.012, -0.12)]
    cable = core.sweep('cable', core.catmull_rom(ctrl, 10), [(0.0028 * math.cos(2 * math.pi * k / 12), 0.0028 * math.sin(2 * math.pi * k / 12)) for k in range(12)])
    core.assign(cable, cable_mat)

    obj = core.join([lamp, cable], 'SM_DeskLamp')
    core.finish_hard_surface(obj, 40)
    core.uv_layout(obj, [
        (core.material_is(obj, 'lamp_paint'), 'box', None, (0.0, 0.0, 1.0, 1.0)),
        (core.material_is(obj, 'lamp_steel'), 'box', None, (0.0, 0.0, 1.0, 1.0)),
        (core.material_is(obj, 'lamp_inside'), 'box', None, (0.0, 0.0, 1.0, 1.0)),
        (core.material_is(obj, 'lamp_bulb'), 'box', None, (0.0, 0.0, 1.0, 1.0)),
        (lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)),
    ])
    return [obj]


def pose_for_review(objs):
    """A stand-in for the desk's rear edge, so the cable reads as dropping over it."""
    bpy.ops.mesh.primitive_cube_add(size=1.0)
    slab = bpy.context.view_layer.objects.active
    slab.name = 'review_desk_edge'
    slab.scale = (0.7, REAR_EDGE + 0.4, 0.03)
    slab.location = (0.0, REAR_EDGE - (REAR_EDGE + 0.4) / 2, -0.015)
    m = core.Mat('review_desk_edge')
    m.set('Base Color', core.hex_linear(0x3b2616))
    m.set('Roughness', 0.45)
    core.assign(slab, m)
