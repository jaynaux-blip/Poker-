"""Parts and finishes shared by the GearDrop props (monitors, the PC, mics, lights, the macro pad...).

Consumer electronics are mostly a handful of materials (textured matte plastic, gloss plastic, anodized
aluminum, rubber, braided cable) on rounded boxes, rods and discs; these keep the assets short and
consistent with each other. Geometry is built in place: transforms go into the mesh data, so every
part can be joined without applying anything.
"""
import math

import bpy  # noqa: F401,I001 (bpy first: the PyPI build registers bmesh and mathutils on import)
import bmesh
from mathutils import Matrix, Vector

from . import core
from .shade import convex_edges, mix_float, noise, object_coords, ramp


# ------------------------------------------------------------------ geometry

def rbox(name, center, size, r=0.0, segments=3, rot=None):
    """A box of full size (x, y, z) with every edge rounded by r, centered at center, optionally rotated
    (a 3x3 Matrix, Euler or Quaternion)."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=Vector(size), verts=bm.verts)
    r = min(r, min(size) * 0.499)
    if r > 0.0:
        bmesh.ops.bevel(bm, geom=bm.edges[:], offset=r, segments=segments, profile=0.5, affect='EDGES', clamp_overlap=True)
    obj = core.mesh_object(name, bm)
    m = rot.to_matrix().to_4x4() if hasattr(rot, 'to_matrix') else (rot.to_4x4() if rot is not None else Matrix())
    obj.data.transform(Matrix.Translation(Vector(center)) @ m)
    return obj


def panel(name, outline, z0, z1, r=0.0, segments=3, matrix=None):
    """A prism from a counter-clockwise XY outline, z0 up to z1, its rim edges rounded by r, then moved by matrix."""
    bm = core.slab(outline, z0, z1)
    if r > 0.0:
        core.bevel(bm, lambda e: abs(e.verts[0].co.z - e.verts[1].co.z) < 1e-7, r, segments)
    obj = core.mesh_object(name, bm)
    if matrix is not None:
        obj.data.transform(matrix)
    return obj


def outline_prism(name, outer, holes=(), z0=0.0, z1=0.001, matrix=None):
    """A flat shape with holes (fan frames, grilles) from z0 up to z1, moved by matrix; transforms applied."""
    obj = core.extrude_outline(name, outer, holes=holes, depth=z1 - z0, z=z0)
    core.apply_transforms(obj)
    core.orient_normals(obj)
    if matrix is not None:
        obj.data.transform(matrix)
    return obj


def annulus(name, r0, r1, z0, z1, n=48, matrix=None):
    """A flat ring (washer) around Z from z0 to z1."""
    return outline_prism(name, [(r1 * math.cos(2 * math.pi * k / n), r1 * math.sin(2 * math.pi * k / n)) for k in range(n)],
                         [[(r0 * math.cos(2 * math.pi * k / n), r0 * math.sin(2 * math.pi * k / n)) for k in range(n)]], z0, z1, matrix)


def circle(r, n):
    return [(r * math.cos(2 * math.pi * k / n), r * math.sin(2 * math.pi * k / n)) for k in range(n)]


def rod(name, a, b, r, n=16):
    """A capped cylinder from point a to point b."""
    return core.sweep(name, [Vector(a), Vector(b)], circle(r, n))


def disc(name, center, axis, r, width, n=32):
    """A short cylinder (knob, axle, foot, lens) centered at center along axis."""
    d = Vector(axis).normalized()
    c = Vector(center)
    return rod(name, c - d * width / 2, c + d * width / 2, r, n)


def tube(name, points, r, n=10, steps=8):
    """A round cable or bent tube through 3D control points (smoothed)."""
    return core.sweep(name, core.catmull_rom([tuple(p) for p in points], steps), circle(r, n))


def spring(name, a, b, coil_r=0.005, wire_r=0.0008, pitch=0.002, lead=0.006):
    """A tension spring from a to b: a tight helix with straight leads at each end."""
    a, b = Vector(a), Vector(b)
    axis = b - a
    length = axis.length
    d = axis.normalized()
    ref = Vector((0, 0, 1)) if abs(d.z) < 0.9 else Vector((1, 0, 0))
    u = d.cross(ref).normalized()
    v = d.cross(u)
    turns = max((length - 2 * lead) / pitch, 1.0)
    steps = int(turns * 10)
    pts = [a]
    for i in range(steps + 1):
        t = i / steps
        ang = t * turns * 2 * math.pi
        pts.append(a + d * (lead + t * (length - 2 * lead)) + u * (coil_r * math.cos(ang)) + v * (coil_r * math.sin(ang)))
    pts.append(b)
    return core.sweep(name, pts, circle(wire_r, 6))


def lathe_at(name, profile, matrix, segments=48):
    """A lathe (see core.lathe) moved into place by matrix; normals made consistent."""
    obj = core.lathe(name, profile, segments=segments)
    core.orient_normals(obj)
    obj.data.transform(matrix)
    return obj


def xform(objs, matrix):
    for o in (objs if isinstance(objs, (list, tuple)) else [objs]):
        o.data.transform(matrix)
    return objs


def frame(origin, forward, up=(0.0, 0.0, 1.0)):
    """A matrix whose -Y axis (a prop's front) points along forward and whose Z is as close to up as it can be."""
    f = Vector(forward).normalized()
    y = -f
    x = y.cross(Vector(up)).normalized()
    z = x.cross(y).normalized()
    m = Matrix((x, y, z)).transposed().to_4x4()
    m.translation = Vector(origin)
    return m


def rot_z(deg):
    return Matrix.Rotation(math.radians(deg), 4, 'Z')


def rot_x(deg):
    return Matrix.Rotation(math.radians(deg), 4, 'X')


def rot_y(deg):
    return Matrix.Rotation(math.radians(deg), 4, 'Y')


def move(x, y, z):
    return Matrix.Translation((x, y, z))


def finish(objs, name, angle=35.0):
    """Joins parts (already carrying their materials) into one object with hard-surface shading and
    a box-projected UV layout per material (each material bakes to its own texture)."""
    obj = core.join(objs, name) if len(objs) > 1 else objs[0]
    obj.name = name
    obj.data.name = name
    core.finish_hard_surface(obj, angle)
    groups = [(core.material_is(obj, s.material.name), 'box', None, (0.0, 0.0, 1.0, 1.0)) for s in obj.material_slots]
    groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
    core.uv_layout(obj, groups)
    return obj


def glow_object(objs, name):
    """Joins the parts that light up (fan rings, key faces, a ring light's diffuser) into one mesh the game
    shades itself (a colour it sets at runtime), so it is exported unbaked with a plain emissive material."""
    obj = core.join(objs, name) if len(objs) > 1 else objs[0]
    obj.name = name
    obj.data.name = name
    obj.data.shade_smooth()
    m = core.Mat(name.replace('SM_', 'glow_').lower())
    m.set('Base Color', (1.0, 1.0, 1.0, 1.0))
    m.set('Emission Color', (1.0, 1.0, 1.0, 1.0))
    m.set('Emission Strength', 1.0)
    m.set('Roughness', 0.5)
    core.assign(obj, m)
    return obj


# ------------------------------------------------------------------ materials

def plastic(name, hex_color, rough=0.5, grain=1.0, scuffs=0.5, scuff_hex=None):
    """Textured injection-molded plastic: a fine grain, a little lighter and smoother on worn convex edges."""
    m = core.Mat(name)
    tc, _ = object_coords(m)
    g = noise(m, tc.outputs['Object'], scale=2400.0, detail=1.0)
    edges = m.math('MULTIPLY', convex_edges(m, tc, distance=0.0012, samples=12, breakup_scale=260.0), scuffs)
    base = core.hex_linear(hex_color)
    lift = core.hex_linear(scuff_hex if scuff_hex is not None else _lighten(hex_color, 0.18))
    m.set('Base Color', m.mix(m.math('MULTIPLY', edges, 0.6), base, lift))
    m.set('Roughness', m.math('SUBTRACT', m.math('ADD', rough, m.math('MULTIPLY', g, 0.06)), m.math('MULTIPLY', edges, 0.12)))
    bump = m.node('ShaderNodeBump', Strength=0.05 * grain, Distance=0.00004)
    m.link(g, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def gloss(name, hex_color, rough=0.08, metal=0.0, coat=0.0):
    """Gloss plastic, glass or lacquer: smooth, with faint micro-scratches in the roughness."""
    m = core.Mat(name)
    tc, _ = object_coords(m)
    scratch = ramp(m, noise(m, tc.outputs['Object'], scale=1600.0, detail=4.0, distortion=2.0), 0.62, 0.7)
    m.set('Base Color', core.hex_linear(hex_color))
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', scratch, 0.12)))
    m.set('Metallic', metal)
    if coat > 0.0:
        m.set('Coat Weight', coat)
    return m


def metal(name, hex_color, rough=0.3, brushed_axis=None, anodized=False):
    """Aluminum or steel: brushed along an axis ('X', 'Y' or 'Z') or bead-blasted; anodized keeps it colored
    and less mirror-like."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    if brushed_axis:
        s = {'X': (8.0, 600.0, 600.0), 'Y': (600.0, 8.0, 600.0), 'Z': (600.0, 600.0, 8.0)}[brushed_axis]
        from .shade import scaled
        streak = noise(m, scaled(m, sep, *s), detail=4.0)
    else:
        streak = noise(m, tc.outputs['Object'], scale=1800.0, detail=2.0)
    m.set('Base Color', m.mix(m.math('MULTIPLY', streak, 0.15), core.hex_linear(hex_color), core.hex_linear(_lighten(hex_color, 0.2))))
    m.set('Metallic', 0.85 if anodized else 1.0)
    m.set('Roughness', m.math('ADD', rough, m.math('MULTIPLY', m.math('SUBTRACT', streak, 0.5), 0.1)))
    return m


def rubber(name, hex_color=0x121212, rough=0.82):
    m = core.Mat(name)
    tc, _ = object_coords(m)
    g = noise(m, tc.outputs['Object'], scale=3000.0, detail=1.0)
    m.set('Base Color', m.mix(m.math('MULTIPLY', g, 0.3), core.hex_linear(hex_color), core.hex_linear(_lighten(hex_color, 0.08))))
    m.set('Roughness', rough)
    return m


def fabric(name, hex_color, period=0.0016, rough=0.88, sheen=0.5, hex_alt=None):
    """A woven fabric: a fine plain weave in the bump, heathered color."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    heather = noise(m, tc.outputs['Object'], scale=900.0, detail=2.0)
    wa = m.math('SINE', m.math('MULTIPLY', m.math('ADD', sep.outputs['X'], sep.outputs['Z']), 2 * math.pi / period))
    wb = m.math('SINE', m.math('MULTIPLY', m.math('ADD', sep.outputs['Y'], sep.outputs['Z']), 2 * math.pi / period))
    weave = m.math('MULTIPLY', m.math('ADD', m.math('MULTIPLY', wa, wb), 1.0), 0.5)
    m.set('Base Color', m.mix(m.math('MULTIPLY', heather, 0.6), core.hex_linear(hex_color), core.hex_linear(hex_alt if hex_alt is not None else _lighten(hex_color, 0.12))))
    m.set('Roughness', rough)
    m.set('Sheen Weight', sheen)
    m.set('Sheen Roughness', 0.45)
    bump = m.node('ShaderNodeBump', Strength=0.3, Distance=0.0003)
    m.link(m.math('ADD', weave, m.math('MULTIPLY', heather, 0.4)), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def braided(name, hex_color=0x1a1a1a):
    """Braided cable sleeve: a twill along the cable."""
    m = core.Mat(name)
    tc, _ = object_coords(m)
    weave = noise(m, tc.outputs['Object'], scale=2400.0, detail=1.0)
    m.set('Base Color', m.mix(m.math('MULTIPLY', weave, 0.4), core.hex_linear(hex_color), core.hex_linear(_lighten(hex_color, 0.12))))
    m.set('Roughness', 0.75)
    bump = m.node('ShaderNodeBump', Strength=0.3, Distance=0.0002)
    m.link(weave, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def emissive(name, hex_color, strength=1.0, base_hex=None, rough=0.4):
    """A part that glows by itself (status LEDs, a power button ring): baked into the emissive texture."""
    m = core.Mat(name)
    m.set('Base Color', core.hex_linear(base_hex if base_hex is not None else hex_color))
    m.set('Emission Color', core.hex_linear(hex_color))
    m.set('Emission Strength', strength)
    m.set('Roughness', rough)
    return m


def screen_glass(name, hex_color=0x07080a):
    """A display switched off (or under a widget): near-black, glossy, anti-glare."""
    m = core.Mat(name)
    tc, _ = object_coords(m)
    m.set('Base Color', core.hex_linear(hex_color))
    m.set('Roughness', m.math('ADD', 0.16, m.math('MULTIPLY', noise(m, tc.outputs['Object'], scale=900.0, detail=2.0), 0.05)))
    m.set('Specular IOR Level', 0.35)
    return m


def _lighten(h, amount):
    r, g, b = (h >> 16) & 255, (h >> 8) & 255, h & 255
    r, g, b = (int(c + (255 - c) * amount) for c in (r, g, b))
    return (r << 16) | (g << 8) | b


def assign_all(objs, mat):
    for o in objs:
        core.assign(o, mat)
    return objs


def mix_mats(m, factor, a, b):
    return mix_float(m, factor, a, b)
