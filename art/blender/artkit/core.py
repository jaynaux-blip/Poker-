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

def cycles_device():
    """'GPU' when Cycles can use an OptiX or CUDA card (much faster bakes and reviews), else 'CPU'.

    Set SHORTSTACK_CPU=1 to force the CPU.
    """
    if os.environ.get('SHORTSTACK_CPU'):
        return 'CPU'
    try:
        prefs = bpy.context.preferences.addons['cycles'].preferences
    except (KeyError, AttributeError):
        return 'CPU'
    for kind in ('OPTIX', 'CUDA'):
        try:
            prefs.compute_device_type = kind
        except TypeError:
            continue
        prefs.get_devices()
        gpus = [d for d in prefs.devices if d.type == kind]
        if gpus:
            for d in prefs.devices:
                d.use = d.type == kind
            return 'GPU'
    return 'CPU'


def reset():
    """Empty scene in meters, rendering with Cycles (on the GPU when there is one)."""
    # The factory startup file only: read_factory_settings would also reset the user's preferences,
    # and Blender then uninstalls the Python wheels of every extension it no longer sees enabled.
    bpy.ops.wm.read_homefile(use_empty=True, use_factory_startup=True)
    sc = bpy.context.scene
    sc.unit_settings.system = 'METRIC'
    sc.unit_settings.scale_length = 1.0
    sc.render.engine = 'CYCLES'
    sc.cycles.device = cycles_device()
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
    """One sRGB channel in 0..1 to linear."""
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def hex_linear(h, a=1.0):
    """0xRRGGBB (sRGB) to a linear RGBA tuple for shader inputs."""
    return (srgb_to_linear(((h >> 16) & 255) / 255.0), srgb_to_linear(((h >> 8) & 255) / 255.0), srgb_to_linear((h & 255) / 255.0), a)


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

def bake(obj, name, size=2048, ao_distance=0.01, ao_samples=64, sizes=None):
    """Bakes every material on obj to BaseColor, ORM (occlusion, roughness, metallic), Normal and,
    for emissive materials, Emissive textures, then replaces the materials with glTF-ready ones.

    sizes: optional {material name: texture size, or (width, height)} overriding size per material.
    """
    sc = bpy.context.scene
    select_only([obj])
    tex_dir = os.path.join(BUILD_DIR, 'textures', name)
    os.makedirs(tex_dir, exist_ok=True)
    sc.world.light_settings.distance = ao_distance
    sizes = sizes or {}
    results = []
    for slot_index, slot in enumerate(obj.material_slots):
        mat = slot.material
        nt = mat.node_tree
        bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
        out = next(n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL')
        surface_src = out.inputs['Surface'].links[0].from_socket
        suffix = f'_{slot_index}' if len(obj.material_slots) > 1 else ''
        px_size = sizes.get(mat.name, size)
        px_w, px_h = px_size if isinstance(px_size, (tuple, list)) else (px_size, px_size)
        strength = bsdf.inputs['Emission Strength'].default_value
        emissive = strength > 0.0 and (bsdf.inputs['Emission Color'].is_linked or any(c > 0.0 for c in bsdf.inputs['Emission Color'].default_value[:3]))
        maps = {}
        for key, non_color in (('BaseColor', False), ('Roughness', True), ('Metallic', True), ('Normal', True), ('AO', True), ('Emissive', False)):
            img = bpy.data.images.new(f'T_{name}{suffix}_{key}', px_w, px_h, alpha=False)
            img.colorspace_settings.name = 'Non-Color' if non_color else 'sRGB'
            maps[key] = img
        target = nt.nodes.new('ShaderNodeTexImage')
        results.append({'mat': mat, 'bsdf': bsdf, 'out': out, 'surface': surface_src, 'target': target, 'maps': maps,
                        'suffix': suffix, 'size': (px_w, px_h), 'emissive': emissive, 'strength': strength})

    def set_targets(key):
        for r in results:
            r['target'].image = r['maps'][key]
            for n in r['mat'].node_tree.nodes:
                n.select = False
            r['target'].select = True
            r['mat'].node_tree.nodes.active = r['target']

    def emit_from(input_name):
        """Temporarily route a Principled input to an Emission shader."""
        tmp = []
        for r in results:
            nt = r['mat'].node_tree
            em = nt.nodes.new('ShaderNodeEmission')
            sock = r['bsdf'].inputs[input_name]
            if input_name == 'Emission Color' and not r['emissive']:
                em.inputs['Color'].default_value = (0.0, 0.0, 0.0, 1.0)
            elif sock.is_linked:
                nt.links.new(sock.links[0].from_socket, em.inputs['Color'])
            else:
                v = sock.default_value
                em.inputs['Color'].default_value = tuple(v) if hasattr(v, '__len__') else (v, v, v, 1.0)
            nt.links.new(em.outputs['Emission'], r['out'].inputs['Surface'])
            tmp.append((nt, em, r['out'], r['surface']))
        return tmp

    def restore(tmp):
        for nt, em, out, surface_src in tmp:
            nt.links.new(surface_src, out.inputs['Surface'])
            nt.nodes.remove(em)

    sc.render.bake.margin = 16
    sc.render.bake.use_clear = True
    sc.cycles.samples = 1
    passes = [('BaseColor', 'Base Color'), ('Roughness', 'Roughness'), ('Metallic', 'Metallic')]
    if any(r['emissive'] for r in results):
        passes.append(('Emissive', 'Emission Color'))
    for key, src in passes:
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
    for r in results:
        w, h = r['size']
        px = {}
        for key in ('Roughness', 'Metallic', 'AO'):
            a = np.empty(w * h * 4, dtype=np.float32)
            r['maps'][key].pixels.foreach_get(a)
            px[key] = a.reshape(h, w, 4)[:, :, 0]
        orm = np.ones((h, w, 4), dtype=np.float32)
        orm[:, :, 0] = np.clip(0.35 + 0.65 * px['AO'], 0, 1)  # keep occlusion gentle: Lumen adds its own
        orm[:, :, 1] = px['Roughness']
        orm[:, :, 2] = px['Metallic']
        orm_img = bpy.data.images.new(f"T_{name}{r['suffix']}_ORM", w, h, alpha=False)
        orm_img.colorspace_settings.name = 'Non-Color'
        orm_img.pixels.foreach_set(orm.ravel())
        saved = {}
        outputs = [('BaseColor', r['maps']['BaseColor']), ('Normal', r['maps']['Normal']), ('ORM', orm_img)]
        if r['emissive']:
            outputs.append(('Emissive', r['maps']['Emissive']))
        for key, img in outputs:
            path = os.path.join(tex_dir, f"T_{name}{r['suffix']}_{key}.png")
            img.filepath_raw = path
            img.file_format = 'PNG'
            img.save()
            saved[key] = path
        new_mats.append(gltf_material(f"M_{name}{r['suffix']}", saved['BaseColor'], saved['ORM'], saved['Normal'],
                                      saved.get('Emissive'), r['strength']))
    for i, m in enumerate(new_mats):
        obj.material_slots[i].material = m
    return new_mats


def gltf_material(name, base_color, orm, normal, emissive=None, emissive_strength=1.0):
    """Principled material wired the way the glTF exporter expects (including occlusion)."""
    mat = Mat(name)
    if emissive:
        em = mat.image(emissive)
        mat.link(em.outputs['Color'], mat.bsdf.inputs['Emission Color'])
        mat.set('Emission Strength', emissive_strength)
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


# ------------------------------------------------------------------ hard-surface helpers

def rounded_rect(x0, y0, x1, y1, r, steps=8, radii=None):
    """Counter-clockwise outline of a rectangle with rounded corners (radii: per corner, starting bottom-left)."""
    rs = radii or (r, r, r, r)
    corners = [((x0 + rs[0], y0 + rs[0]), rs[0], math.pi), ((x1 - rs[1], y0 + rs[1]), rs[1], 1.5 * math.pi),
               ((x1 - rs[2], y1 - rs[2]), rs[2], 0.0), ((x0 + rs[3], y1 - rs[3]), rs[3], 0.5 * math.pi)]
    pts = []
    for (cx, cy), cr, a0 in corners:
        if cr <= 0:
            pts.append((cx, cy))
            continue
        for k in range(steps + 1):
            a = a0 + 0.5 * math.pi * k / steps
            pts.append((cx + cr * math.cos(a), cy + cr * math.sin(a)))
    return pts


def slab(outline, z0, z1):
    """A bmesh prism: an n-gon outline (counter-clockwise, in XY) from z0 up to z1."""
    bm = bmesh.new()
    vs = [bm.verts.new((x, y, z0)) for x, y in outline]
    face = bm.faces.new(vs)
    res = bmesh.ops.extrude_face_region(bm, geom=[face])
    top = [g for g in res['geom'] if isinstance(g, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, verts=top, vec=(0.0, 0.0, z1 - z0))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return bm


def bevel(bm, pred, offset, segments=3, profile=0.5):
    edges = [e for e in bm.edges if pred(e)]
    if edges:
        bmesh.ops.bevel(bm, geom=edges, offset=offset, segments=segments, profile=profile, affect='EDGES', clamp_overlap=True)


def prism_x(name, outline, x0, x1):
    """A prism along X from an outline given as (y, z) points: cutters for side ports."""
    bm = bmesh.new()
    a = [bm.verts.new((x0, y, z)) for y, z in outline]
    b = [bm.verts.new((x1, y, z)) for y, z in outline]
    bm.faces.new(a)
    bm.faces.new(list(reversed(b)))
    n = len(outline)
    for i in range(n):
        bm.faces.new((a[i], a[(i + 1) % n], b[(i + 1) % n], b[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return mesh_object(name, bm)


def sweep(name, path, profile, closed=False, caps=True):
    """Sweeps a closed 2D profile [(a, b), ...] along a 3D path of points: handles, rods, tubes, springs.

    Frames are parallel-transported along the path, so the profile never flips. UVs: u runs around
    the profile, v along the path by arc length.
    """
    pts = [Vector(p) for p in path]
    n = len(pts)
    tangents = []
    for i in range(n):
        a = pts[i - 1] if (closed or i > 0) else pts[i]
        b = pts[(i + 1) % n] if (closed or i < n - 1) else pts[i]
        tangents.append((b - a).normalized())
    ref = Vector((0.0, 0.0, 1.0)) if abs(tangents[0].z) < 0.9 else Vector((1.0, 0.0, 0.0))
    normal = (ref - tangents[0] * ref.dot(tangents[0])).normalized()
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    rings = []
    for i in range(n):
        if i > 0:
            normal = tangents[i - 1].rotation_difference(tangents[i]) @ normal
            normal = (normal - tangents[i] * normal.dot(tangents[i])).normalized()
        binormal = tangents[i].cross(normal)
        rings.append([bm.verts.new(pts[i] + normal * a + binormal * b) for a, b in profile])
    lengths = [0.0]
    for i in range(1, n + (1 if closed else 0)):
        lengths.append(lengths[-1] + (pts[i % n] - pts[i - 1]).length)
    total = lengths[-1] or 1.0
    m = len(profile)
    for i in range(n if closed else n - 1):
        r0, r1 = rings[i], rings[(i + 1) % n]
        v0, v1 = lengths[i] / total, lengths[i + 1] / total
        for k in range(m):
            f = bm.faces.new((r0[k], r0[(k + 1) % m], r1[(k + 1) % m], r1[k]))
            for loop, t in zip(f.loops, ((k / m, v0), ((k + 1) / m, v0), ((k + 1) / m, v1), (k / m, v1))):
                loop[uv].uv = t
    if caps and not closed:
        bm.faces.new(list(reversed(rings[0])))
        bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return mesh_object(name, bm)


def catmull_rom(points, steps=8):
    """A smooth curve through 2D or 3D control points (ends included), steps samples per span."""
    pts = [Vector(p) for p in points]
    out = []
    for i in range(len(pts) - 1):
        p0, p1, p2, p3 = pts[max(i - 1, 0)], pts[i], pts[i + 1], pts[min(i + 2, len(pts) - 1)]
        for k in range(steps):
            t = k / steps
            out.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t))
    out.append(pts[-1])
    return [tuple(p) for p in out]


def orient_normals(obj, toward=None):
    """Makes face normals consistent (outward on a closed mesh); with toward, flips an open
    surface so its normals face that direction on average."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    if toward is not None:
        bm.normal_update()
        if sum((f.normal.dot(Vector(toward)) * f.calc_area() for f in bm.faces)) < 0:
            bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()


def loft(name, rings, caps=True):
    """A surface through closed rings of 3D points (all the same count), capped at both ends:
    tapered legs, spines, anything whose section changes along its length."""
    bm = bmesh.new()
    verts = [[bm.verts.new(p) for p in ring] for ring in rings]
    m = len(rings[0])
    for a, b in zip(verts, verts[1:]):
        for k in range(m):
            bm.faces.new((a[k], a[(k + 1) % m], b[(k + 1) % m], b[k]))
    if caps:
        bm.faces.new(list(reversed(verts[0])))
        bm.faces.new(verts[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return mesh_object(name, bm)


def subdivide(obj, levels=2):
    """Applies Catmull-Clark subdivision: soft forms (cushions, pads) from a low cage."""
    mod = obj.modifiers.new('subsurf', 'SUBSURF')
    mod.levels = levels
    mod.render_levels = levels
    select_only([obj])
    bpy.ops.object.modifier_apply(modifier=mod.name)


def finish_hard_surface(obj, angle=35.0, weighted=True):
    """Smooth shading, with edges sharper than angle kept crisp.

    weighted: face-area weighted normals (applied), so large flat faces stay flat and the bevels
    around them take the shading transition, as in production hard-surface work.
    """
    obj.data.shade_smooth()
    try:
        obj.data.set_sharp_from_angle(angle=math.radians(angle))
    except AttributeError:
        pass
    if weighted:
        mod = obj.modifiers.new('weighted', 'WEIGHTED_NORMAL')
        mod.mode = 'FACE_AREA'
        mod.keep_sharp = True
        mod.weight = 50
        select_only([obj])
        bpy.ops.object.modifier_apply(modifier=mod.name)


def boolean(obj, cutter, op='DIFFERENCE'):
    mod = obj.modifiers.new('bool', 'BOOLEAN')
    mod.operation = op
    mod.solver = 'EXACT'
    mod.object = cutter
    mod.material_mode = 'TRANSFER'
    select_only([obj])
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(cutter)


def keycap(bm, x, y, w, d, z0, z1, radius=0.0009, taper=0.0006):
    """A chiclet keycap (bmesh, added to bm): rounded footprint w x d at z0, slightly smaller at the top z1."""
    outline = rounded_rect(x - w / 2, y - d / 2, x + w / 2, y + d / 2, radius * 1.4, steps=3)
    vs0 = [bm.verts.new((px, py, z0)) for px, py in outline]
    sx = (w - 2 * taper) / w
    sy = (d - 2 * taper) / d
    vs1 = [bm.verts.new((x + (px - x) * sx, y + (py - y) * sy, z1)) for px, py in outline]
    bm.faces.new(list(reversed(vs0)))
    top = bm.faces.new(vs1)
    n = len(outline)
    for i in range(n):
        bm.faces.new((vs0[i], vs0[(i + 1) % n], vs1[(i + 1) % n], vs1[i]))
    return top


# ------------------------------------------------------------------ UV layouts

def uv_layout(obj, groups, margin=0.01):
    """Lays out UVs by face groups, in order; a face takes the first group whose test accepts it.

    Each group is (test(face) -> bool, mode, arg, rect), rect = (u0, v0, u1, v1):
      ('planar', (axis_a, axis_b, a0, a1, b0, b1)): projects onto two axes ('x', 'y' or 'z', optionally '-x'
        to mirror), mapping [a0, a1] x [b0, b1] onto rect. Used where print must stay sharp.
      ('box', gap or None): each face projects along its dominant normal axis; connected faces sharing an
        axis form islands, shelf-packed into rect at one uniform texel density (gap in UV units).
      ('smart', None): smart-projected islands packed into rect.
      ('keep', None): the faces' existing UVs (from lathe or sweep), scaled from [0, 1]^2 into rect.
      ('cylinder', (z0, z1)): around the Z axis (u = angle, from -X counterclockwise) by height, for
        bands such as a chip's edge.
    """
    me = obj.data
    if not me.uv_layers:
        me.uv_layers.new(name='UVMap')
    select_only([obj])
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_mode(type='FACE')
    bm = bmesh.from_edit_mesh(me)
    bm.faces.ensure_lookup_table()
    owner = [None] * len(bm.faces)
    for f in bm.faces:
        for gi, (test, mode, arg, rect) in enumerate(groups):
            if test(f):
                owner[f.index] = gi
                break
    axis_index = {'x': 0, 'y': 1, 'z': 2}
    for gi, (test, mode, arg, rect) in enumerate(groups):
        u0, v0, u1, v1 = rect
        if mode == 'smart':
            for f in bm.faces:
                f.select_set(owner[f.index] == gi)
            bmesh.update_edit_mesh(me)
            if not any(o == gi for o in owner):
                continue
            bpy.ops.uv.smart_project(angle_limit=math.radians(66.0), island_margin=margin)
            bm = bmesh.from_edit_mesh(me)
            bm.faces.ensure_lookup_table()
            uv = bm.loops.layers.uv.verify()
            for f in bm.faces:
                if owner[f.index] == gi:
                    for l in f.loops:
                        l[uv].uv = (u0 + (u1 - u0) * l[uv].uv.x, v0 + (v1 - v0) * l[uv].uv.y)
        elif mode == 'keep':
            uv = bm.loops.layers.uv.verify()
            for f in bm.faces:
                if owner[f.index] == gi:
                    for l in f.loops:
                        l[uv].uv = (u0 + (u1 - u0) * l[uv].uv.x, v0 + (v1 - v0) * l[uv].uv.y)
        elif mode == 'cylinder':
            z0, z1 = arg
            uv = bm.loops.layers.uv.verify()
            faces_us = []
            for f in bm.faces:
                if owner[f.index] != gi:
                    continue
                us = [math.atan2(l.vert.co.y, l.vert.co.x) / (2 * math.pi) + 0.5 for l in f.loops]
                if max(us) - min(us) > 0.5:  # the face straddles the seam: unwrap it past 1
                    us = [u + 1.0 if u < 0.5 else u for u in us]
                faces_us.append((f, us))
            # Squeeze the wrap so the seam faces stay inside the rectangle (outside it is unbaked).
            span = max([1.0] + [max(us) for _, us in faces_us])
            for f, us in faces_us:
                for l, u in zip(f.loops, us):
                    l[uv].uv = (u0 + (u1 - u0) * u / span, v0 + (v1 - v0) * (l.vert.co.z - z0) / (z1 - z0))
        elif mode == 'box':
            uv = bm.loops.layers.uv.verify()
            _box_project([f for f in bm.faces if owner[f.index] == gi], uv, rect, arg if arg is not None else margin * 0.4)
        else:
            a_name, b_name, a0, a1, b0, b1 = arg
            ia = axis_index[a_name.lstrip('-')]
            ib = axis_index[b_name.lstrip('-')]
            sa = -1.0 if a_name.startswith('-') else 1.0
            sb = -1.0 if b_name.startswith('-') else 1.0
            uv = bm.loops.layers.uv.verify()
            for f in bm.faces:
                if owner[f.index] == gi:
                    for l in f.loops:
                        ta = (sa * l.vert.co[ia] - a0) / (a1 - a0)
                        tb = (sb * l.vert.co[ib] - b0) / (b1 - b0)
                        l[uv].uv = (u0 + (u1 - u0) * ta, v0 + (v1 - v0) * tb)
    bmesh.update_edit_mesh(me)
    bpy.ops.object.mode_set(mode='OBJECT')


# Projection plane per dominant axis and sign, chosen so no island is mirrored: (u axis, u sign, v axis, v sign).
_BOX_PLANES = {(0, True): (1, 1.0, 2, 1.0), (0, False): (1, -1.0, 2, 1.0),
               (1, True): (0, -1.0, 2, 1.0), (1, False): (0, 1.0, 2, 1.0),
               (2, True): (0, 1.0, 1, 1.0), (2, False): (0, 1.0, 1, -1.0)}


def _box_project(faces, uv, rect, gap):
    if not faces:
        return
    key = {}
    for f in faces:
        n = f.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        key[f.index] = (ax, n[ax] > 0)
    # Islands: faces connected by edges and sharing a projection.
    seen = set()
    islands = []
    for f in faces:
        if f.index in seen:
            continue
        seen.add(f.index)
        stack, comp = [f], []
        while stack:
            g = stack.pop()
            comp.append(g)
            for e in g.edges:
                for h in e.link_faces:
                    if h.index not in seen and h.index in key and key[h.index] == key[g.index]:
                        seen.add(h.index)
                        stack.append(h)
        plane = _BOX_PLANES[key[f.index]]
        ia, sa, ib, sb = plane
        pts = [(sa * v.co[ia], sb * v.co[ib]) for g in comp for v in g.verts]
        a0 = min(p[0] for p in pts)
        a1 = max(p[0] for p in pts)
        b0 = min(p[1] for p in pts)
        b1 = max(p[1] for p in pts)
        rotate = (b1 - b0) > (a1 - a0)  # lay tall islands down: shelves pack wide ones best
        islands.append({'faces': comp, 'plane': plane, 'box': (a0, a1, b0, b1), 'rotate': rotate})
    u0, v0, u1, v1 = rect
    W, H = u1 - u0, v1 - v0

    def dims(isl, s):
        a0, a1, b0, b1 = isl['box']
        w, h = (a1 - a0) * s, (b1 - b0) * s
        return (h, w) if isl['rotate'] else (w, h)

    def pack(s):
        order = sorted(range(len(islands)), key=lambda i: -dims(islands[i], s)[1])
        x = y = shelf = 0.0
        pos = [None] * len(islands)
        for i in order:
            w, h = dims(islands[i], s)
            if w > W:
                return None
            if x + w > W:
                y += shelf + gap
                x = shelf = 0.0
            if y + h > H:
                return None
            pos[i] = (x, y)
            x += w + gap
            shelf = max(shelf, h)
        return pos

    area = sum((i['box'][1] - i['box'][0]) * (i['box'][3] - i['box'][2]) for i in islands)
    lo, hi = 0.0, math.sqrt(W * H / max(area, 1e-12))
    for _ in range(40):
        mid = (lo + hi) / 2
        if pack(mid) is not None:
            lo = mid
        else:
            hi = mid
    s = lo
    pos = pack(s)
    for isl, (px, py) in zip(islands, pos):
        a0, a1, b0, b1 = isl['box']
        ia, sa, ib, sb = isl['plane']
        for g in isl['faces']:
            for l in g.loops:
                a, b = sa * l.vert.co[ia], sb * l.vert.co[ib]
                if isl['rotate']:
                    x, y = (b - b0) * s, (a1 - a) * s
                else:
                    x, y = (a - a0) * s, (b - b0) * s
                l[uv].uv = (u0 + px + x, v0 + py + y)


def normal_z(f, above=0.9):
    return f.normal.z > above


def material_is(obj, name):
    idx = next((i for i, s in enumerate(obj.material_slots) if s.material and s.material.name == name), -1)
    return lambda f: f.material_index == idx
