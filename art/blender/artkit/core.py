"""Scene, geometry, material, baking and export helpers."""
import math
import os

import bpy  # noqa: I001 (bpy first: the PyPI build registers bmesh and mathutils on import)
import bmesh
import numpy as np
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
FONTS = os.path.join(ROOT, 'art', 'fonts')
EXPORT_DIR = os.path.join(ROOT, 'unreal', 'Art', 'Meshes')
REVIEW_DIR = os.path.join(ROOT, 'art', 'review')
BUILD_DIR = os.path.join(ROOT, 'art', 'build')


# ------------------------------------------------------------------ scene

def reset():
    """Empty scene in meters, rendering with Cycles on the CPU."""
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc = bpy.context.scene
    sc.unit_settings.system = 'METRIC'
    sc.unit_settings.scale_length = 1.0
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    sc.cycles.use_denoising = True
    sc.cycles.use_adaptive_sampling = True
    sc.view_settings.view_transform = 'AgX'
    world = bpy.data.worlds.new('World')
    sc.world = world
    return sc


def link(obj):
    bpy.context.scene.collection.objects.link(obj)
    return obj


def select_only(objs, active=None):
    for o in bpy.context.scene.objects:
        o.select_set(False)
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = active or objs[0]


def srgb_to_linear(c):
    c = c / 255.0 if c > 1.0 else c
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def hex_linear(h, a=1.0):
    """0xRRGGBB (sRGB) to a linear RGBA tuple for shader inputs."""
    return (srgb_to_linear((h >> 16) & 255), srgb_to_linear((h >> 8) & 255), srgb_to_linear(h & 255), a)


# ------------------------------------------------------------------ geometry

def mesh_object(name, bm):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    return link(obj)


def lathe(name, profile, segments=128, v_range=(0.0, 1.0), u_range=(0.0, 1.0)):
    """Revolve (radius, z) points, listed bottom to top, around Z.

    A point with radius 0 closes the surface at the axis. Normals face left of
    the direction of travel (outward for a profile that climbs the outside).
    UVs: u runs around the axis, v along the profile by arc length, so a label
    maps without distortion.
    """
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    rings = []
    for r, z in profile:
        if r <= 1e-7:
            rings.append([bm.verts.new((0.0, 0.0, z))])
        else:
            rings.append([bm.verts.new((r * math.cos(2 * math.pi * i / segments), r * math.sin(2 * math.pi * i / segments), z)) for i in range(segments)])
    lengths = [0.0]
    for (r0, z0), (r1, z1) in zip(profile, profile[1:]):
        lengths.append(lengths[-1] + math.hypot(r1 - r0, z1 - z0))
    total = lengths[-1] or 1.0
    vs = [v_range[0] + (v_range[1] - v_range[0]) * (l / total) for l in lengths]

    def u_of(i):
        return u_range[0] + (u_range[1] - u_range[0]) * (i / segments)

    for j in range(len(rings) - 1):
        a, b = rings[j], rings[j + 1]
        for i in range(segments):
            i1 = i + 1
            if len(a) == 1 and len(b) == 1:
                continue
            if len(a) == 1:
                f = bm.faces.new((a[0], b[i1 % segments], b[i]))
                uvs = [((u_of(i) + u_of(i1)) / 2, vs[j]), (u_of(i1), vs[j + 1]), (u_of(i), vs[j + 1])]
            elif len(b) == 1:
                f = bm.faces.new((a[i], a[i1 % segments], b[0]))
                uvs = [(u_of(i), vs[j]), (u_of(i1), vs[j]), ((u_of(i) + u_of(i1)) / 2, vs[j + 1])]
            else:
                f = bm.faces.new((a[i], a[i1 % segments], b[i1 % segments], b[i]))
                uvs = [(u_of(i), vs[j]), (u_of(i1), vs[j]), (u_of(i1), vs[j + 1]), (u_of(i), vs[j + 1])]
            for loop, t in zip(f.loops, uvs):
                loop[uv].uv = t
    obj = mesh_object(name, bm)
    obj.data.shade_smooth()
    return obj


def fillet(p0, corner, p1, radius, steps=6):
    """Points rounding the corner between segments p0-corner and corner-p1 (2D tuples)."""
    c = Vector(corner)
    d0 = (Vector(p0) - c).normalized()
    d1 = (Vector(p1) - c).normalized()
    ang = math.acos(max(-1.0, min(1.0, d0.dot(d1))))
    if ang < 1e-3 or ang > math.pi - 1e-3:
        return [corner]
    t = radius / math.tan(ang / 2)
    a = c + d0 * t
    b = c + d1 * t
    pts = []
    for k in range(steps + 1):
        s = k / steps
        # quadratic Bezier through the corner approximates the arc closely enough for bevels
        q = (1 - s) ** 2 * a + 2 * (1 - s) * s * c + s ** 2 * b
        pts.append((q.x, q.y))
    return pts


def rounded_profile(points, radii, steps=6):
    """Polyline with rounded corners: radii[i] rounds points[i] (0 keeps it sharp; ends are kept)."""
    out = [points[0]]
    for i in range(1, len(points) - 1):
        if radii[i] > 0:
            out.extend(fillet(points[i - 1], points[i], points[i + 1], radii[i], steps))
        else:
            out.append(points[i])
    out.append(points[-1])
    return out


def extrude_outline(name, outer, holes=(), depth=0.001, z=0.0, bevel=0.0):
    """A flat shape (outer outline and optional holes, 2D points in XY) extruded along +Z."""
    curve = bpy.data.curves.new(name, 'CURVE')
    curve.dimensions = '2D'
    curve.fill_mode = 'BOTH'
    curve.extrude = depth / 2
    curve.bevel_depth = bevel
    curve.bevel_resolution = 3 if bevel > 0 else 0
    for loop in [outer, *holes]:
        sp = curve.splines.new('POLY')
        sp.points.add(len(loop) - 1)
        for p, (x, y) in zip(sp.points, loop):
            p.co = (x, y, 0.0, 1.0)
        sp.use_cyclic_u = True
    tmp = link(bpy.data.objects.new(name + '_curve', curve))
    tmp.location.z = z + depth / 2
    select_only([tmp])
    bpy.ops.object.convert(target='MESH')
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    obj.data.name = name
    return obj


def circle_points(cx, cy, r, n=48, start=0.0):
    return [(cx + r * math.cos(start + 2 * math.pi * i / n), cy + r * math.sin(start + 2 * math.pi * i / n)) for i in range(n)]


def smart_uv(obj, margin=0.02, angle=66.0):
    select_only([obj])
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(angle), island_margin=margin)
    bpy.ops.object.mode_set(mode='OBJECT')


def join(objs, name):
    select_only(objs, objs[0])
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    obj.data.name = name
    return obj


def apply_transforms(obj):
    select_only([obj])
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)


# ------------------------------------------------------------------ materials

class Mat:
    """A node material with small helpers for building shader graphs."""

    def __init__(self, name):
        self.m = bpy.data.materials.new(name)
        if not self.m.node_tree:
            self.m.use_nodes = True
        self.nt = self.m.node_tree
        self.bsdf = next((n for n in self.nt.nodes if n.type == 'BSDF_PRINCIPLED'), None) or self.nt.nodes.new('ShaderNodeBsdfPrincipled')
        self.out = next((n for n in self.nt.nodes if n.type == 'OUTPUT_MATERIAL'), None) or self.nt.nodes.new('ShaderNodeOutputMaterial')
        if not self.out.inputs['Surface'].is_linked:
            self.nt.links.new(self.bsdf.outputs['BSDF'], self.out.inputs['Surface'])

    def node(self, kind, **values):
        n = self.nt.nodes.new(kind)
        for k, v in values.items():
            if k.startswith('_'):
                setattr(n, k[1:], v)
            else:
                n.inputs[k].default_value = v
        return n

    def link(self, src, dst):
        self.nt.links.new(src, dst)

    def math(self, op, a, b=None, clamp=False):
        n = self.nt.nodes.new('ShaderNodeMath')
        n.operation = op
        n.use_clamp = clamp
        for i, v in enumerate((a, b)):
            if v is None:
                continue
            if isinstance(v, (int, float)):
                n.inputs[i].default_value = v
            else:
                self.link(v, n.inputs[i])
        return n.outputs[0]

    def mix(self, factor, a, b, blend='MIX'):
        n = self.nt.nodes.new('ShaderNodeMix')
        n.data_type = 'RGBA'
        n.blend_type = blend
        for sock, v in ((n.inputs[0], factor), (n.inputs[6], a), (n.inputs[7], b)):
            if isinstance(v, (int, float)):
                sock.default_value = v
            elif isinstance(v, tuple):
                sock.default_value = v
            else:
                self.link(v, sock)
        return n.outputs[2]

    def image(self, path, non_color=False, vector=None, extension='REPEAT'):
        n = self.nt.nodes.new('ShaderNodeTexImage')
        n.image = bpy.data.images.load(path, check_existing=True)
        n.image.colorspace_settings.name = 'Non-Color' if non_color else 'sRGB'
        n.extension = extension
        if vector is not None:
            self.link(vector, n.inputs['Vector'])
        return n

    def set(self, name, value):
        """Principled input: a constant or a socket."""
        if isinstance(value, (int, float, tuple)):
            self.bsdf.inputs[name].default_value = value
        else:
            self.link(value, self.bsdf.inputs[name])


def assign(obj, mat):
    obj.data.materials.clear()
    obj.data.materials.append(mat.m if isinstance(mat, Mat) else mat)


# ------------------------------------------------------------------ baking

def bake(obj, name, size=2048, ao_distance=0.01, ao_samples=64):
    """Bakes every material on obj to BaseColor, ORM (occlusion, roughness, metallic) and Normal
    textures, then replaces the materials with glTF-ready ones that use those textures."""
    sc = bpy.context.scene
    select_only([obj])
    tex_dir = os.path.join(BUILD_DIR, 'textures', name)
    os.makedirs(tex_dir, exist_ok=True)
    sc.world.light_settings.distance = ao_distance
    results = []
    for slot_index, slot in enumerate(obj.material_slots):
        mat = slot.material
        nt = mat.node_tree
        bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
        out = next(n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL')
        surface_src = out.inputs['Surface'].links[0].from_socket
        suffix = f'_{slot_index}' if len(obj.material_slots) > 1 else ''
        maps = {}
        for key, non_color in (('BaseColor', False), ('Roughness', True), ('Metallic', True), ('Normal', True), ('AO', True)):
            img = bpy.data.images.new(f'T_{name}{suffix}_{key}', size, size, alpha=False)
            img.colorspace_settings.name = 'Non-Color' if non_color else 'sRGB'
            maps[key] = img
        target = nt.nodes.new('ShaderNodeTexImage')
        results.append((mat, bsdf, out, surface_src, target, maps, suffix))

    def set_targets(key):
        for mat, bsdf, out, surface_src, target, maps, suffix in results:
            target.image = maps[key]
            for n in mat.node_tree.nodes:
                n.select = False
            target.select = True
            mat.node_tree.nodes.active = target

    def emit_from(input_name):
        """Temporarily route a Principled input to an Emission shader."""
        tmp = []
        for mat, bsdf, out, surface_src, target, maps, suffix in results:
            nt = mat.node_tree
            em = nt.nodes.new('ShaderNodeEmission')
            sock = bsdf.inputs[input_name]
            if sock.is_linked:
                nt.links.new(sock.links[0].from_socket, em.inputs['Color'])
            else:
                v = sock.default_value
                em.inputs['Color'].default_value = tuple(v) if hasattr(v, '__len__') else (v, v, v, 1.0)
            nt.links.new(em.outputs['Emission'], out.inputs['Surface'])
            tmp.append((nt, em, out, surface_src))
        return tmp

    def restore(tmp):
        for nt, em, out, surface_src in tmp:
            nt.links.new(surface_src, out.inputs['Surface'])
            nt.nodes.remove(em)

    sc.render.bake.margin = 16
    sc.render.bake.use_clear = True
    sc.cycles.samples = 1
    for key, src in (('BaseColor', 'Base Color'), ('Roughness', 'Roughness'), ('Metallic', 'Metallic')):
        tmp = emit_from(src)
        set_targets(key)
        bpy.ops.object.bake(type='EMIT')
        restore(tmp)
    sc.cycles.samples = 4
    set_targets('Normal')
    sc.render.bake.normal_space = 'TANGENT'
    bpy.ops.object.bake(type='NORMAL')
    sc.cycles.samples = ao_samples
    set_targets('AO')
    bpy.ops.object.bake(type='AO')

    new_mats = []
    for mat, bsdf, out, surface_src, target, maps, suffix in results:
        px = {}
        for key in ('Roughness', 'Metallic', 'AO'):
            a = np.empty(size * size * 4, dtype=np.float32)
            maps[key].pixels.foreach_get(a)
            px[key] = a.reshape(size, size, 4)[:, :, 0]
        orm = np.ones((size, size, 4), dtype=np.float32)
        orm[:, :, 0] = np.clip(0.35 + 0.65 * px['AO'], 0, 1)  # keep occlusion gentle: Lumen adds its own
        orm[:, :, 1] = px['Roughness']
        orm[:, :, 2] = px['Metallic']
        orm_img = bpy.data.images.new(f'T_{name}{suffix}_ORM', size, size, alpha=False)
        orm_img.colorspace_settings.name = 'Non-Color'
        orm_img.pixels.foreach_set(orm.ravel())
        saved = {}
        for key, img in (('BaseColor', maps['BaseColor']), ('Normal', maps['Normal']), ('ORM', orm_img)):
            path = os.path.join(tex_dir, f'T_{name}{suffix}_{key}.png')
            img.filepath_raw = path
            img.file_format = 'PNG'
            img.save()
            saved[key] = path
        new_mats.append(gltf_material(f'M_{name}{suffix}', saved['BaseColor'], saved['ORM'], saved['Normal']))
    for i, m in enumerate(new_mats):
        obj.material_slots[i].material = m
    return new_mats


def gltf_material(name, base_color, orm, normal):
    """Principled material wired the way the glTF exporter expects (including occlusion)."""
    mat = Mat(name)
    base = mat.image(base_color)
    mat.link(base.outputs['Color'], mat.bsdf.inputs['Base Color'])
    orm_n = mat.image(orm, non_color=True)
    sep = mat.node('ShaderNodeSeparateColor')
    mat.link(orm_n.outputs['Color'], sep.inputs['Color'])
    mat.link(sep.outputs['Green'], mat.bsdf.inputs['Roughness'])
    mat.link(sep.outputs['Blue'], mat.bsdf.inputs['Metallic'])
    nrm = mat.image(normal, non_color=True)
    nmap = mat.node('ShaderNodeNormalMap')
    mat.link(nrm.outputs['Color'], nmap.inputs['Color'])
    mat.link(nmap.outputs['Normal'], mat.bsdf.inputs['Normal'])
    # Occlusion: the exporter reads it from a group named "glTF Material Output".
    group = bpy.data.node_groups.get('glTF Material Output')
    if group is None:
        group = bpy.data.node_groups.new('glTF Material Output', 'ShaderNodeTree')
        group.interface.new_socket('Occlusion', in_out='INPUT', socket_type='NodeSocketFloat')
    gnode = mat.nt.nodes.new('ShaderNodeGroup')
    gnode.node_tree = group
    mat.link(sep.outputs['Red'], gnode.inputs['Occlusion'])
    return mat.m


# ------------------------------------------------------------------ export

def export_glb(objs, name):
    os.makedirs(EXPORT_DIR, exist_ok=True)
    path = os.path.join(EXPORT_DIR, f'{name}.glb')
    select_only(objs)
    bpy.ops.export_scene.gltf(filepath=path, export_format='GLB', use_selection=True, export_apply=True, export_yup=True,
                              export_texcoords=True, export_normals=True, export_materials='EXPORT', export_image_format='AUTO')
    return path


def bounds(objs):
    pts = [o.matrix_world @ Vector(c) for o in objs for c in o.bound_box]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return lo, hi


def stats(objs):
    tris = 0
    for o in objs:
        me = o.evaluated_get(bpy.context.evaluated_depsgraph_get()).to_mesh()
        me.calc_loop_triangles()
        tris += len(me.loop_triangles)
    lo, hi = bounds(objs)
    return tris, hi - lo
