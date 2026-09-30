"""SM_Chair: a cheap mid-back mesh office chair that has done a few thousand sessions.

- A five-star nylon base on twin-wheel casters, with a gas lift in a telescoping shroud.
- A charcoal fabric seat, pilled at the front edge and shiny where it's sat on.
- A curved mesh back on a plastic frame.
- T-armrests whose cheap PU skin is peeling at the front.

Coordinates (meters): origin on the floor under the gas lift, Z up, the seat's front toward -Y.
The stage turns it to face the desk.
"""
import math
import random

import bpy  # noqa: I001 (bpy first: the PyPI build registers bmesh and mathutils on import)
import bmesh
from mathutils import Matrix, Vector

from artkit import core
from artkit.shade import convex_edges, mix_float, noise, object_coords, ramp, scaled

MESHES = ['SM_Chair']
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {'chair_fabric': 2048, 'chair_mesh': 2048, 'chair_plastic': 2048}
AO_DISTANCE = 0.04

SEAT_Z = 0.50            # top of the cushion
BACK_TILT = math.radians(9)

REVIEW_VIEWS = [('front', -35, 18, 2.2), ('back', 150, 22, 2.2), ('detail', -30, 30, 0.55, (0.24, -0.12, 0.68))]


# ------------------------------------------------------------------ materials

def fabric_material():
    m = core.Mat('chair_fabric')
    tc, sep = object_coords(m)
    heather = noise(m, tc.outputs['Object'], scale=1400.0, detail=2.0)
    weave_a = m.math('SINE', m.math('MULTIPLY', m.math('ADD', sep.outputs['X'], sep.outputs['Z']), 2 * math.pi / 0.0016))
    weave_b = m.math('SINE', m.math('MULTIPLY', m.math('ADD', sep.outputs['Y'], sep.outputs['Z']), 2 * math.pi / 0.0016))
    weave = m.math('MULTIPLY', m.math('ADD', m.math('MULTIPLY', weave_a, weave_b), 1.0), 0.5)
    color = m.mix(m.math('MULTIPLY', heather, 0.6), core.hex_linear(0x26272b), core.hex_linear(0x3a3c41))
    # Where it gets sat on the fibers flatten and shine; the front edge pills and goes fuzzy-light.
    sat = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ramp(m, m.math('ADD', m.math('MULTIPLY', sep.outputs['X'], sep.outputs['X']),
                                                                     m.math('MULTIPLY', m.math('ADD', sep.outputs['Y'], 0.02), m.math('ADD', sep.outputs['Y'], 0.02))), 0.005, 0.03)),
                 ramp(m, sep.outputs['Z'], SEAT_Z - 0.01, SEAT_Z))
    front = m.math('MULTIPLY', ramp(m, m.math('MULTIPLY', sep.outputs['Y'], -1.0), 0.19, 0.25),
                   ramp(m, noise(m, tc.outputs['Object'], scale=300.0, detail=3.0), 0.45, 0.7))
    color = m.mix(m.math('MULTIPLY', front, 0.45), color, core.hex_linear(0x55565b))
    rough = m.math('SUBTRACT', m.math('ADD', 0.86, m.math('MULTIPLY', heather, 0.06)), m.math('MULTIPLY', sat, 0.22))
    m.set('Base Color', color)
    m.set('Roughness', rough)
    m.set('Sheen Weight', 0.5)
    m.set('Sheen Roughness', 0.4)
    bump = m.node('ShaderNodeBump', Strength=0.35, Distance=0.0003)
    m.link(m.math('ADD', weave, m.math('MULTIPLY', heather, 0.5)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def mesh_material():
    """The back's woven mesh: a coarse open weave, dark threads over darker gaps."""
    m = core.Mat('chair_mesh')
    tc, sep = object_coords(m)
    period = 0.0034
    gx = m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', sep.outputs['X'], period)), 0.5))
    gz = m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', sep.outputs['Z'], period * 0.8)), 0.5))
    hole = m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', 0.5, gx), 0.14, 0.2), ramp(m, m.math('SUBTRACT', 0.5, gz), 0.14, 0.2))
    thread = m.math('SUBTRACT', 1.0, hole)
    color = m.mix(hole, core.hex_linear(0x202124), core.hex_linear(0x050505))
    m.set('Base Color', color)
    m.set('Roughness', mix_float(m, hole, 0.55, 0.9))
    bump = m.node('ShaderNodeBump', Strength=0.6, Distance=0.0006)
    m.link(thread, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def plastic_material():
    m = core.Mat('chair_plastic')
    tc, sep = object_coords(m)
    grain = noise(m, tc.outputs['Object'], scale=1800.0, detail=1.0)
    scuff = m.math('MULTIPLY', convex_edges(m, tc, distance=0.002, samples=12, breakup_scale=200.0), 0.7)
    # Shoes scuff the base legs.
    low = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ramp(m, sep.outputs['Z'], 0.1, 0.16)),
                 ramp(m, noise(m, scaled(m, sep, 60.0, 60.0, 8.0), detail=3.0), 0.55, 0.72))
    color = m.mix(m.math('MAXIMUM', scuff, m.math('MULTIPLY', low, 0.6)), core.hex_linear(0x161618), core.hex_linear(0x44454a))
    m.set('Base Color', color)
    m.set('Roughness', m.math('ADD', 0.48, m.math('MULTIPLY', grain, 0.08)))
    bump = m.node('ShaderNodeBump', Strength=0.06, Distance=0.00005)
    m.link(grain, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def pu_material():
    """Armpad skin: black PU leatherette, peeling at the front where forearms drag."""
    m = core.Mat('chair_pu')
    tc, sep = object_coords(m)
    grain = noise(m, tc.outputs['Object'], scale=2600.0, detail=2.0)
    # Peeling only along the front edge, in small flakes over a dull gray fabric backing.
    peel_zone = ramp(m, m.math('MULTIPLY', sep.outputs['Y'], -1.0), 0.112, 0.128)
    flakes = ramp(m, noise(m, tc.outputs['Object'], scale=420.0, detail=4.0), 0.56, 0.6)
    peel = m.math('MULTIPLY', peel_zone, flakes)
    color = m.mix(peel, m.mix(m.math('MULTIPLY', grain, 0.3), core.hex_linear(0x121213), core.hex_linear(0x1c1c1e)), core.hex_linear(0x3b3936))
    m.set('Base Color', color)
    m.set('Roughness', mix_float(m, peel, m.math('ADD', 0.42, m.math('MULTIPLY', grain, 0.1)), 0.85))
    bump = m.node('ShaderNodeBump', Strength=0.25, Distance=0.0002)
    m.link(m.math('SUBTRACT', m.math('MULTIPLY', grain, 0.3), peel), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def chrome_material():
    m = core.Mat('chair_chrome')
    tc, _ = object_coords(m)
    m.set('Base Color', core.hex_linear(0xc9ccd1))
    m.set('Metallic', 1.0)
    m.set('Roughness', m.math('ADD', 0.1, m.math('MULTIPLY', noise(m, tc.outputs['Object'], scale=300.0, detail=3.0), 0.1)))
    return m


# ------------------------------------------------------------------ geometry

def rrect_ring(center, u, v, half_u, half_v, r, steps=4):
    """A rounded-rectangle section in the plane spanned by unit vectors u and v."""
    return [center + u * px + v * py for px, py in core.rounded_rect(-half_u, -half_v, half_u, half_v, min(r, half_u, half_v) * 0.999, steps=steps)]


def star_base(plastic):
    parts = []
    hub = core.lathe('hub', [(0.0, 0.085), (0.034, 0.085), (0.040, 0.095), (0.040, 0.118), (0.032, 0.126), (0.0, 0.126)], segments=48)
    core.orient_normals(hub)
    parts.append(hub)
    tips = []
    for k in range(5):
        a = math.radians(90 + k * 72)
        d = Vector((math.cos(a), math.sin(a), 0.0))
        side = Vector((-d.y, d.x, 0.0))
        rings = []
        for t in (0.0, 0.25, 0.55, 0.85, 1.0):
            reach = 0.02 + t * 0.29
            top = 0.122 - t * 0.035
            half_w = 0.022 - t * 0.006
            h = 0.034 - t * 0.012
            c = d * reach + Vector((0, 0, top - h / 2))
            rings.append(rrect_ring(c, side, Vector((0, 0, 1)), half_w, h / 2, 0.008))
        leg = core.loft('leg', rings)
        parts.append(leg)
        tips.append((d * 0.30, a))
    base = core.join(parts, 'star')
    core.assign(base, plastic)
    return base, tips


def caster(tip, heading, plastic, chrome):
    """Twin-wheel caster under a leg tip, swiveled to heading."""
    parts = []
    rot = Matrix.Rotation(heading, 4, 'Z')
    for s in (-1, 1):
        wheel = core.lathe('wheel', [(0.0, -0.0065), (0.022, -0.0065), (0.025, -0.0045), (0.025, 0.0045), (0.022, 0.0065), (0.0, 0.0065)], segments=32)
        core.orient_normals(wheel)
        wheel.data.transform(Matrix.Translation((0.0, 0.0, s * 0.0095)))
        wheel.data.transform(Matrix.Rotation(math.pi / 2, 4, 'Y'))  # axle along X
        wheel.data.transform(Matrix.Translation((0.0, -0.012, 0.025)))
        parts.append(wheel)
    hood_rings = []
    for t, (w, h) in zip((0.0, 0.5, 1.0), ((0.030, 0.012), (0.032, 0.020), (0.020, 0.014))):
        y = -0.034 + t * 0.042
        hood_rings.append(rrect_ring(Vector((0.0, y, 0.040)), Vector((1, 0, 0)), Vector((0, 0, 1)), w / 2, h / 2, 0.006))
    parts.append(core.loft('hood', hood_rings))
    stem = core.lathe('stem', [(0.0, 0.040), (0.0065, 0.040), (0.0065, 0.072), (0.0, 0.072)], segments=16)
    core.orient_normals(stem)
    parts.append(stem)
    c = core.join(parts, 'caster')
    c.data.transform(Matrix.Translation(Vector((tip.x, tip.y, 0.0))) @ rot)
    core.assign(c, plastic)
    return c


def lift(plastic, chrome):
    shroud = core.lathe('shroud', [(0.0, 0.118), (0.031, 0.118), (0.031, 0.195), (0.029, 0.197), (0.029, 0.262), (0.027, 0.264),
                                   (0.027, 0.315), (0.024, 0.318), (0.0, 0.318)], segments=48)
    core.orient_normals(shroud)
    core.assign(shroud, plastic)
    piston = core.lathe('piston', [(0.0, 0.31), (0.0140, 0.31), (0.0140, 0.405), (0.0, 0.405)], segments=32)
    core.orient_normals(piston)
    core.assign(piston, chrome)
    return [shroud, piston]


def mechanism(plastic, chrome):
    bm = core.slab(core.rounded_rect(-0.09, -0.11, 0.09, 0.07, 0.015, steps=4), 0.400, 0.448)
    core.bevel(bm, lambda e: all(abs(v.co.z - 0.448) < 1e-6 or abs(v.co.z - 0.400) < 1e-6 for v in e.verts), 0.004, segments=2)
    body = core.mesh_object('mech', bm)
    core.assign(body, plastic)
    # Height lever on the right, ending in a paddle.
    lever = core.sweep('lever', core.catmull_rom([(0.07, -0.05, 0.415), (0.14, -0.06, 0.415), (0.20, -0.09, 0.42), (0.225, -0.11, 0.425)], 6),
                       [(0.0035 * math.cos(2 * math.pi * k / 10), 0.0035 * math.sin(2 * math.pi * k / 10)) for k in range(10)])
    core.assign(lever, chrome)
    pbm = core.slab(core.rounded_rect(0.215, -0.135, 0.245, -0.095, 0.008, steps=4), 0.418, 0.432)
    paddle = core.mesh_object('paddle', pbm)
    core.assign(paddle, plastic)
    return [body, lever, paddle]


def seat(fabric, plastic):
    # Cushion: a low cage with extra loops near the edges, subdivided into a soft pillow.
    xs = [-0.25, -0.22, -0.12, 0.0, 0.12, 0.22, 0.25]
    ys = [-0.245, -0.215, -0.10, 0.0, 0.10, 0.20, 0.235]
    bm = bmesh.new()
    grid_top, grid_bot = {}, {}
    for i, x in enumerate(xs):
        for j, y in enumerate(ys):
            edge = i in (0, len(xs) - 1) or j in (0, len(ys) - 1)
            front = j == 0
            dome = 0.0 if edge else 0.010 * (1 - (x / 0.25) ** 2) * (1 - ((y + 0.02) / 0.24) ** 2)
            ztop = SEAT_Z - (0.030 if edge else 0.0) - (0.012 if front else 0.0) + dome
            grid_top[i, j] = bm.verts.new((x, y, ztop))
            grid_bot[i, j] = bm.verts.new((x * 0.97, y * 0.97, SEAT_Z - 0.075))
    n, m = len(xs), len(ys)
    for i in range(n - 1):
        for j in range(m - 1):
            bm.faces.new((grid_top[i, j], grid_top[i + 1, j], grid_top[i + 1, j + 1], grid_top[i, j + 1]))
            bm.faces.new((grid_bot[i, j + 1], grid_bot[i + 1, j + 1], grid_bot[i + 1, j], grid_bot[i, j]))
    ring = [(i, 0) for i in range(n)] + [(n - 1, j) for j in range(1, m)] + [(i, m - 1) for i in range(n - 2, -1, -1)] + [(0, j) for j in range(m - 2, 0, -1)]
    for a, b in zip(ring, ring[1:] + ring[:1]):
        bm.faces.new((grid_top[a], grid_bot[a], grid_bot[b], grid_top[b]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    cushion = core.mesh_object('cushion', bm)
    core.subdivide(cushion, 2)
    core.assign(cushion, fabric)
    shell = core.mesh_object('shell', core.slab(core.rounded_rect(-0.235, -0.225, 0.235, 0.22, 0.04, steps=6), SEAT_Z - 0.09, SEAT_Z - 0.072))
    core.assign(shell, plastic)
    return [cushion, shell]


def back(mesh_mat, plastic):
    """Curved mesh back on a swept frame, leaning back BACK_TILT, with the spine that carries it."""
    w, h = 0.46, 0.50
    y0, z0 = 0.265, 0.575

    def place(u, v, inset=0.0):
        """Point on the back's surface: u across (-1..1), v up (0..1); curved in plan and at the lumbar."""
        x = u * (w / 2 - inset)
        z = v * (h - 2 * inset) + inset
        y = 0.045 * (u ** 2) - 0.018 * math.sin(math.pi * min(1.0, v * 1.25))
        p = Vector((x, y, z))
        p = Matrix.Rotation(-BACK_TILT, 3, 'X') @ p
        return p + Vector((0.0, y0, z0))

    # Frame: a rounded-rectangle loop swept with an oval section.
    loop = []
    for u, v in core.rounded_rect(-1.0, 0.0, 1.0, 1.0, 0.14, steps=6):
        loop.append(place(u, v))
    frame = core.sweep('frame', loop, [(0.011 * math.cos(2 * math.pi * k / 12), 0.016 * math.sin(2 * math.pi * k / 12)) for k in range(12)], closed=True)
    core.assign(frame, plastic)
    # The mesh panel: a thin curved sheet spanning the frame.
    bm = bmesh.new()
    nu, nv = 16, 14
    front = [[bm.verts.new(place(-0.97 + 1.94 * i / nu, 0.03 + 0.94 * j / nv)) for j in range(nv + 1)] for i in range(nu + 1)]
    for i in range(nu):
        for j in range(nv):
            bm.faces.new((front[i][j], front[i + 1][j], front[i + 1][j + 1], front[i][j + 1]))
    bmesh.ops.solidify(bm, geom=bm.faces[:], thickness=0.003)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    panel = core.mesh_object('panel', bm)
    core.assign(panel, mesh_mat)
    # Spine from the mechanism, back and up into the frame's bottom rail.
    spine_path = core.catmull_rom([(0.0, 0.06, 0.425), (0.0, 0.2, 0.43), (0.0, 0.29, 0.47), (0.0, 0.30, 0.55), tuple(place(0.0, 0.06))], 8)
    spine = core.sweep('spine', spine_path, rrect_2d(0.028, 0.008, 0.006))
    core.assign(spine, plastic)
    return [frame, panel, spine]


def rrect_2d(half_a, half_b, r, steps=4):
    return core.rounded_rect(-half_a, -half_b, half_a, half_b, r, steps=steps)


def arms(plastic, pu):
    parts = []
    for s in (-1, 1):
        path = core.catmull_rom([(s * 0.10, 0.02, 0.43), (s * 0.20, 0.02, 0.43), (s * 0.262, 0.02, 0.46), (s * 0.272, 0.02, 0.55), (s * 0.272, 0.02, 0.665)], 8)
        post = core.sweep('arm_post', path, rrect_2d(0.012, 0.024, 0.006))
        core.assign(post, plastic)
        parts.append(post)
        bm = core.slab(core.rounded_rect(s * 0.272 - 0.036, -0.13, s * 0.272 + 0.036, 0.11, 0.03, steps=8), 0.665, 0.692)
        core.bevel(bm, lambda e: all(abs(v.co.z - 0.692) < 1e-6 for v in e.verts), 0.009, segments=4)
        core.bevel(bm, lambda e: all(abs(v.co.z - 0.665) < 1e-6 for v in e.verts), 0.003, segments=2)
        pad = core.mesh_object('arm_pad', bm)
        core.assign(pad, pu)
        parts.append(pad)
    return parts


def build():
    core.reset()
    fabric, mesh_mat, plastic, pu, chrome = fabric_material(), mesh_material(), plastic_material(), pu_material(), chrome_material()
    base, tips = star_base(plastic)
    parts = [base]
    rnd = random.Random(5)
    for tip, a in tips:
        parts.append(caster(tip, a + math.pi / 2 + rnd.uniform(-1.2, 1.2), plastic, chrome))
    parts += lift(plastic, chrome) + mechanism(plastic, chrome) + seat(fabric, plastic) + back(mesh_mat, plastic) + arms(plastic, pu)
    for p in parts:
        core.finish_hard_surface(p, 45)
    obj = core.join(parts, 'SM_Chair')
    core.uv_layout(obj, [
        (core.material_is(obj, name), 'box', None, (0.0, 0.0, 1.0, 1.0))
        for name in ('chair_fabric', 'chair_mesh', 'chair_plastic', 'chair_pu', 'chair_chrome')
    ])
    return [obj]
