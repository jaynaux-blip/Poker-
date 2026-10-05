"""SM_DR_*: Dee's front room, upstairs over the Wash & Fold, where the evicted player sleeps on her couch; and
SM_Umbrella, for the street's people in the rain.

Every mesh: meters, Z up, its front toward Blender -Y (a viewer in front of it stands on its -Y side). Blender +X is UE
+X and Blender Y is UE -Y (the glTF import mirrors Y and flips handedness, so prints that read in Blender read in game).
Free-standing props have their origin on the floor at the middle of their footprint. The window and the door have it on
the floor at the middle of their opening, on the wall's room face (y = 0; their jambs run into +Y); the kitchenette's
back is the wall (y = +0.31, where its calendar hangs).

Slots are baked (M_<mesh>_<index>, in the order SLOTS prints at build time) except the unbaked _Glow meshes (one white
emissive slot for the game to color) and SM_DR_Window_Glass (dr_window_glass). Two-sided slots: TWO_SIDED.

- SM_DR_Couch (2.27 x 1.04 x 0.95 m with the throw; the couch itself 2.10 x 0.91 x 0.87): a three-seat rust corduroy
  couch (wide wales running with each panel, bald on the arms and seat fronts, a cigarette burn on the right arm),
  rolled arms, loose cushions with welts, the middle seat broken down; a crocheted granny-square throw (cloth
  simulated) over the left arm and back corner, Dee's floral tapestry and mustard velvet pillows left of the middle,
  the player's striped bed pillow under a folded army blanket at the right end. Seat tops about 0.47 (middle 0.42).
- SM_DR_CoffeeTable (1.10 x 0.60 x 0.45): honey oak, ring stains, scratches; the deck and its box, three cards, the
  ashtray that holds Dee's keys (a Riverside $5 chip on the ring), a cork coaster; magazines, the Northside Herald and
  the crossword on the shelf. LAPTOP_POINT is where the laptop sits (the middle of the top is kept clear).
- SM_DR_Armchair (0.90 x 0.93 x 1.03): Dee's brown vinyl recliner, cracked and flaking where she sits; her oatmeal
  cable-knit cardigan over the left arm (cloth simulated), one sleeve down the side, the other across the seat.
- SM_DR_TVStand (0.96 x 0.45 stand, the set to 1.13, 1.60 m to the antenna tips) + SM_DR_TVStand_Glow (the CRT's face,
  UV 0..1).
- SM_DR_Rug (2.40 x 1.70, 8 mm): a worn Heriz-style machine-made rug (rug_pattern), fringed ends, a traffic path
  worn to the foundation in places, a curled corner (+X/-Y) and a soft buckle across the -X end (R_BUCKLE).
- SM_DR_FloorLamp (1.56) + SM_DR_FloorLamp_Glow (the bulb and the shade's lining); the shade's outside glows in its
  own baked emissive (slot 1, drl_shade, strength 5). L_BULB is the bulb's middle.
- SM_DR_Radiator (0.98 x 0.19 x 0.71 with its pan), SM_DR_Shelf (0.80 x 0.30 x 1.80, the pothos above it),
  SM_DR_Plant (a snake plant, 0.85), SM_DR_Kitchenette (1.20 x 0.62 x 0.91, the calendar to 1.55),
  SM_DR_Window (+ SM_DR_Window_Glass), SM_DR_Door, SM_Umbrella (1.0 m across, origin at the grip, canopy up +Z).

DR_ONLY=couch,table,... (environment) builds just those (keys of BUILDERS); DR_REVIEW=room,props picks the extra
review sheets (art/review/dee_room_room.jpg: every prop in a lamp-lit room; dee_room_props_N.jpg: close looks).
"""
import math
import os
import random
import time
import traceback

import bpy  # noqa: F401,I001
import bmesh
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts, review, street, texmaps
from artkit.shade import convex_edges, image_surface, mix_float, noise, object_coords, ramp, scaled, smooth_less, vec
from artkit.sheet import Sheet

MESHES = ['SM_DR_Couch', 'SM_DR_CoffeeTable', 'SM_DR_Armchair', 'SM_DR_TVStand', 'SM_DR_TVStand_Glow', 'SM_DR_Rug',
          'SM_DR_FloorLamp', 'SM_DR_FloorLamp_Glow', 'SM_DR_Radiator', 'SM_DR_Shelf', 'SM_DR_Plant', 'SM_DR_Kitchenette',
          'SM_DR_Window', 'SM_DR_Window_Glass', 'SM_DR_Door', 'SM_Umbrella']
UNBAKED = {'SM_DR_TVStand_Glow', 'SM_DR_FloorLamp_Glow', 'SM_DR_Window_Glass'}
_ONLY = [s.strip() for s in os.environ.get('DR_ONLY', '').split(',') if s.strip()]
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {
    'drc_cord': 4096, 'drc_throw': 2048, 'drc_soft': 2048, 'drc_wood': 256,
    'drt_wood': 2048, 'drt_print': 2048, 'drt_glass': 512, 'drt_metal': 256, 'drt_misc': 256,
    'dra_vinyl': 2048, 'dra_knit': 2048, 'dra_misc': 256,
    'drv_lam': 1024, 'drv_plastic': 1024, 'drv_print': 2048, 'drv_screen': 512, 'drv_metal': 256,
    'drr_pile': 2048, 'drr_fringe': 512, 'drl_brass': 1024, 'drl_shade': 1024, 'drl_cord': 256,
    'drh_paint': 2048, 'drh_pan': 512, 'drh_valve': 256,
    'drs_wood': 1024, 'drs_woodv': 1024, 'drs_print': 2048, 'drs_body': 1024, 'drs_metal': 256, 'drs_plant': 1024,
    'drp_leaf': 1024, 'drp_pot': 512,
    'drk_paint': 1024, 'drk_counter': 1024, 'drk_steel': 512, 'drk_enamel': 1024, 'drk_print': 2048, 'drk_rack': 512, 'drk_towel': 512,
    'drw_paint': 2048, 'drw_brass': 128, 'drw_plant': 512, 'drw_shade': 512,
    'drd_paint': 2048, 'drd_metal': 256, 'drd_stone': 256,
    'dru_canopy': 1024, 'dru_frame': 256, 'dru_handle': 256,
}
# Materials exported two-sided (single sheets: cloth, leaves, the umbrella's canopy).
TWO_SIDED = {'drc_throw', 'dra_knit', 'drs_plant', 'drk_towel', 'dru_canopy'}
AO_DISTANCE = 0.06
REVIEW_VIEWS = None
REVIEW_SCREEN_LIGHT = False
SCRATCH = os.path.join(core.BUILD_DIR, 'dee_room')

# Where the laptop sits on SM_DR_CoffeeTable (meters, Blender, the table's frame): the middle of the top.
LAPTOP_POINT = (0.0, 0.03, 0.45)

SLOTS = {}        # mesh name -> its slot material names, in order (for the report and the two-sided fix)
ALL = []          # every object build() made, for assemble()


def H(h):
    return core.hex_linear(h)


# ================================================================== geometry helpers

def _samples(h, r, step, arcs=(15.0, 30.0)):
    """Grid coordinates across [-h, h] for a rounded box: even inside, the rounding's arc split in equal angles."""
    inner = max(h - r, 0.0)
    ends = [inner + r * math.tan(math.radians(a)) for a in arcs] + [h]
    n = int(math.ceil(2 * inner / step)) if inner > 1e-6 else 0
    mid = [-inner + 2 * inner * k / n for k in range(n + 1)] if n else [0.0]
    return [-e for e in reversed(ends)] + mid + ends


def soft_box(name, half, r, step=0.03, deform=None, matrix=None, arcs=(15.0, 30.0)):
    """A rounded box (half extents, edge radius r) as an even quad grid; deform(P, N) -> P reshapes it (numpy, local
    coordinates, N the rounding's outward normals) before matrix places it. Returns the object."""
    half = tuple(float(h) for h in half)
    r = min(r, min(half) * 0.999)
    ax = [_samples(h, r, step, arcs) for h in half]
    bm = bmesh.new()
    cache = {}

    def vert(p):
        key = tuple(round(c, 6) for c in p)
        v = cache.get(key)
        if v is None:
            v = bm.verts.new(p)
            cache[key] = v
        return v

    for k in range(3):
        a, b = [i for i in range(3) if i != k]
        for s in (-1.0, 1.0):
            A, B = ax[a], ax[b]
            for i in range(len(A) - 1):
                for j in range(len(B) - 1):
                    quad = []
                    for ii, jj in ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1)):
                        p = [0.0, 0.0, 0.0]
                        p[k] = s * half[k]
                        p[a] = A[ii]
                        p[b] = B[jj]
                        quad.append(vert(tuple(p)))
                    try:
                        bm.faces.new(quad)
                    except ValueError:
                        pass
    bm.verts.ensure_lookup_table()
    P = np.array([v.co[:] for v in bm.verts])
    P, N = _round(P, half, r)
    if deform is not None:
        P = deform(P, N)
    for v, p in zip(bm.verts, P):
        v.co = p
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    obj = core.mesh_object(name, bm)
    obj.data.shade_smooth()
    if matrix is not None:
        obj.data.transform(matrix)
    return obj


def _round(P, half, r):
    inner = np.maximum(np.array(half) - r, 0.0)
    C = np.clip(P, -inner, inner)
    D = P - C
    n = np.linalg.norm(D, axis=1, keepdims=True)
    N = D / np.maximum(n, 1e-9)
    return C + r * N, N


def soft_loop(half, r, axis, sign, deform=None, spacing=0.008):
    """The seam around one face of a soft_box (where a welt runs): points and outward normals, closed loop."""
    half = tuple(float(h) for h in half)
    r = min(r, min(half) * 0.999)
    a, b = [i for i in range(3) if i != axis]
    ha, hb = half[a], half[b]
    corners = [(-ha, -hb), (ha, -hb), (ha, hb), (-ha, hb)]
    pts = []
    for (a0, b0), (a1, b1) in zip(corners, corners[1:] + corners[:1]):
        L = math.hypot(a1 - a0, b1 - b0)
        n = max(2, int(L / spacing))
        for k in range(n):
            t = k / n
            p = [0.0, 0.0, 0.0]
            p[axis] = sign * half[axis]
            p[a] = a0 + (a1 - a0) * t
            p[b] = b0 + (b1 - b0) * t
            pts.append(p)
    P = np.array(pts)
    P, N = _round(P, half, r)
    if deform is not None:
        P = deform(P, N)
    return P, N


def welt(name, P, N, radius=0.0042, out=0.6, matrix=None, closed=True, n=8):
    """A piping cord along a loop of points (pushed out along N by out * radius)."""
    pts = [Vector(p) + Vector(q) * radius * out for p, q in zip(P, N)]
    obj = core.sweep(name, pts, parts.circle(radius, n), closed=closed)
    if matrix is not None:
        obj.data.transform(matrix)
    return obj


def offset_poly(pts, d):
    """A closed counter-clockwise 2D polygon moved inward by d along its vertex normals (mitred, clamped)."""
    n = len(pts)
    out = []
    for i in range(n):
        p0, p, p1 = Vector(pts[i - 1]), Vector(pts[i]), Vector(pts[(i + 1) % n])
        e0 = (p - p0).normalized()
        e1 = (p1 - p).normalized()
        l0 = Vector((-e0.y, e0.x))
        l1 = Vector((-e1.y, e1.x))
        nm = l0 + l1
        if nm.length < 1e-6:
            nm = l0
        nm.normalize()
        out.append(tuple(p + nm * (d / max(nm.dot(l0), 0.35))))
    return out


def ccw(pts):
    area = sum(x0 * y1 - x1 * y0 for (x0, y0), (x1, y1) in zip(pts, pts[1:] + pts[:1]))
    return pts if area > 0 else list(reversed(pts))


def round_poly(pts, radii, steps=5):
    """A closed polygon with corner i rounded by radii[i]."""
    out = []
    n = len(pts)
    for i in range(n):
        if radii[i] > 0:
            out.extend(core.fillet(pts[i - 1], pts[i], pts[(i + 1) % n], radii[i], steps))
        else:
            out.append(pts[i])
    return out


def round_prism(name, prof, y0, y1, r0=0.0, r1=0.0, steps=5, matrix=None):
    """A prism along +Y of a closed (x, z) profile from y0 to y1, its ends' edges rounded by r0 / r1 (the profile
    inset along its normals toward each cap)."""
    prof = ccw([tuple(p) for p in prof])
    rings = []
    if r0 > 0:
        for k in range(steps + 1):
            a = k / steps * math.pi / 2
            rings.append((y0 + r0 * (1 - math.cos(a)), r0 * (1 - math.sin(a))))
    else:
        rings.append((y0, 0.0))
    if r1 > 0:
        for k in range(steps, -1, -1):
            a = k / steps * math.pi / 2
            rings.append((y1 - r1 * (1 - math.cos(a)), r1 * (1 - math.sin(a))))
    else:
        rings.append((y1, 0.0))
    bm = bmesh.new()
    vs = []
    for y, inset in rings:
        poly = offset_poly(prof, inset) if inset > 1e-7 else prof
        vs.append([bm.verts.new((x, y, z)) for x, z in poly])
    m = len(prof)
    for a, b in zip(vs, vs[1:]):
        for k in range(m):
            bm.faces.new((a[k], a[(k + 1) % m], b[(k + 1) % m], b[k]))
    caps = [bm.faces.new(list(reversed(vs[0]))), bm.faces.new(vs[-1])]
    bmesh.ops.triangulate(bm, faces=caps, quad_method='BEAUTY', ngon_method='BEAUTY')
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    obj = core.mesh_object(name, bm)
    obj.data.shade_smooth()
    if matrix is not None:
        obj.data.transform(matrix)
    return obj


def lathe(name, profile, seg=48, matrix=None):
    return parts.lathe_at(name, profile, matrix if matrix is not None else Matrix(), seg)


def rbox(name, center, size, r=0.0, seg=2, rot=None):
    return parts.rbox(name, center, size, r, seg, rot)


def box(x0, x1, y0, y1, z0, z1, r=0.0, seg=2, name='box'):
    return parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), r, seg)


def rod(a, b, r, n=16, name='rod'):
    return parts.rod(name, a, b, r, n)


def tube(points, r, n=10, steps=8, name='tube'):
    return parts.tube(name, points, r, n, steps)


def mat4(loc=(0, 0, 0), rz=0.0, rx=0.0, ry=0.0):
    return (Matrix.Translation(Vector(loc)) @ Matrix.Rotation(math.radians(rz), 4, 'Z') @ Matrix.Rotation(math.radians(ry), 4, 'Y')
            @ Matrix.Rotation(math.radians(rx), 4, 'X'))


def xf(obj, matrix):
    obj.data.transform(matrix)
    return obj


def reverse_faces(obj):
    """Turns every face of obj over (after a mirroring transform, which flips the winding)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()
    return obj


def verts_np(obj):
    me = obj.data
    a = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('co', a)
    return a.reshape(-1, 3)


def set_verts(obj, P):
    obj.data.vertices.foreach_set('co', np.asarray(P, dtype=np.float64).ravel())
    obj.data.update()


def soft_clamp(obj, point, normal, give=0.0):
    """Pushes every vertex behind a plane (point, normal) back onto it (a pillow pressed flat where it rests)."""
    P = verts_np(obj)
    n = np.array(normal, float)
    n /= np.linalg.norm(n)
    d = (P - np.array(point)) @ n
    behind = d < 0
    P[behind] -= np.outer(d[behind] * (1.0 - give), n)
    set_verts(obj, P)
    return obj


def set_attr(obj, name, value):
    me = obj.data
    a = me.attributes.get(name) or me.attributes.new(name, 'FLOAT', 'FACE')
    a.data.foreach_set('value', [float(value)] * len(me.polygons))
    return obj


def kind(obj, k):
    """Tags faces with a sub-material index the slot's shader switches on (attribute dr_k)."""
    return set_attr(obj, 'dr_k', k)


def ensure_uv(obj):
    me = obj.data
    if 'UVMap' not in me.uv_layers:
        if len(me.uv_layers):
            me.uv_layers[0].name = 'UVMap'
        else:
            me.uv_layers.new(name='UVMap')
    return me.uv_layers['UVMap']


def set_pat(obj, fn):
    """Sets the 'pat' UV layer (the print or pattern coordinates the shader samples) per loop: fn(co) -> (u, v)."""
    ensure_uv(obj)
    me = obj.data
    layer = me.uv_layers.get('pat') or me.uv_layers.new(name='pat')
    co = verts_np(obj)
    vi = np.empty(len(me.loops), dtype=np.int64)
    me.loops.foreach_get('vertex_index', vi)
    uv = np.array([fn(co[i]) for i in vi], dtype=np.float32)
    layer.data.foreach_set('uv', uv.ravel())
    me.uv_layers.active = me.uv_layers['UVMap']
    return obj


def set_pat_array(obj, uv_per_vertex):
    ensure_uv(obj)
    me = obj.data
    layer = me.uv_layers.get('pat') or me.uv_layers.new(name='pat')
    vi = np.empty(len(me.loops), dtype=np.int64)
    me.loops.foreach_get('vertex_index', vi)
    layer.data.foreach_set('uv', np.asarray(uv_per_vertex, dtype=np.float32)[vi].ravel())
    me.uv_layers.active = me.uv_layers['UVMap']
    return obj


def lock_uv(obj, uv_per_vertex=None):
    """Keeps the layout UVs this object already has (or uv_per_vertex) through finish(): the bake writes into them."""
    if uv_per_vertex is not None:
        me = obj.data
        lay = ensure_uv(obj)
        vi = np.empty(len(me.loops), dtype=np.int64)
        me.loops.foreach_get('vertex_index', vi)
        lay.data.foreach_set('uv', np.asarray(uv_per_vertex, dtype=np.float32)[vi].ravel())
    return set_attr(obj, 'dr_uvlock', 1.0)


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def order_slots(obj, mats):
    me = obj.data
    old = [s.material for s in obj.material_slots]
    remap = []
    for mt in old:
        if mt not in mats:
            raise RuntimeError(f'{obj.name}: material {mt.name if mt else None} not in its slot list')
        remap.append(mats.index(mt))
    fi = np.zeros(len(me.polygons), dtype=np.int32)
    me.polygons.foreach_get('material_index', fi)
    new = np.array(remap, dtype=np.int32)[fi] if remap else fi
    me.materials.clear()
    for mt in mats:
        me.materials.append(mt)
    me.polygons.foreach_set('material_index', new)
    used = set(new.tolist())
    for i, mt in enumerate(mats):
        if i not in used:
            print(f'[dee_room] WARNING {obj.name}: slot {i} ({mt.name}) has no faces')


def finish(objs, name, mats, angle=35.0, weighted=True, rects=None):
    """Joins parts (materials assigned), orders the slots as mats, shades, lays out UVs: faces flagged dr_uvlock keep
    theirs, the rest are box-projected per slot."""
    mats = [m.m if isinstance(m, core.Mat) else m for m in mats]
    for o in objs:
        ensure_uv(o)
        if not o.data.materials:
            raise RuntimeError(f'{name}: part {o.name} has no material')
    obj = core.join(objs, name) if len(objs) > 1 else objs[0]
    obj.name = name
    obj.data.name = name
    me = obj.data
    me.uv_layers.active = me.uv_layers['UVMap']
    me.uv_layers['UVMap'].active_render = True
    order_slots(obj, mats)
    core.finish_hard_surface(obj, angle, weighted)
    lock = me.attributes.get('dr_uvlock')
    locked = set()
    if lock is not None:
        vals = np.zeros(len(me.polygons), dtype=np.float32)
        lock.data.foreach_get('value', vals)
        locked = set(np.nonzero(vals > 0.5)[0].tolist())
    rects = rects or {}
    groups = []
    if locked:
        groups.append((lambda f: f.index in locked, 'keep', None, (0.0, 0.0, 1.0, 1.0)))
    for s in obj.material_slots:
        groups.append((core.material_is(obj, s.material.name), 'box', None, rects.get(s.material.name, (0.0, 0.0, 1.0, 1.0))))
    groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
    core.uv_layout(obj, groups)
    me.uv_layers.active = me.uv_layers['UVMap']
    SLOTS[name] = [m.name for m in mats]
    print(f'[dee_room] {name} slots: {SLOTS[name]}  tris {tri_count(obj)}')
    return obj


def glow(objs, name):
    o = parts.glow_object(objs, name)
    for oo in [o]:
        ensure_uv(oo)
    print(f'[dee_room] {name} tris {tri_count(o)}')
    return o


# ================================================================== cloth

def drape(name, size, cell, matrix, colliders, frames=90, mass=0.35, bend=0.6, self_collide=True, gap=0.004, friction=6.0,
          subdiv=1):
    """A sheet (size meters, quads about `cell` across) dropped from matrix onto the colliders and simulated as cloth.
    Returns the draped mesh in world coordinates with UVMap over the sheet (u along its x, v along its y)."""
    nx, ny = max(2, int(round(size[0] / cell))), max(2, int(round(size[1] / cell)))
    # The colliders first (fresh objects in the scene with a collision modifier), then the sheet.
    cols = []
    for c in colliders:
        me = c.data.copy()
        me.update()
        d = core.link(bpy.data.objects.new(c.name + '_col', me))
        d.matrix_world = c.matrix_world
        d.modifiers.new('col', 'COLLISION')
        d.collision.thickness_outer = gap
        d.collision.cloth_friction = friction
        d.collision.damping = 0.6
        cols.append(d)
    # the floor, so nothing that slides off falls forever
    fl = bpy.data.meshes.new('drape_floor')
    fl.from_pydata([(-6, -6, 0), (6, -6, 0), (6, 6, 0), (-6, 6, 0)], [], [(0, 1, 2, 3)])
    d = core.link(bpy.data.objects.new('drape_floor', fl))
    d.modifiers.new('col', 'COLLISION')
    d.collision.thickness_outer = gap
    d.collision.cloth_friction = friction
    cols.append(d)
    bpy.context.view_layer.update()
    bpy.ops.mesh.primitive_grid_add(x_subdivisions=nx, y_subdivisions=ny, size=1.0, calc_uvs=True)
    sheet = bpy.context.view_layer.objects.active
    sheet.name = name
    sheet.data.transform(Matrix.Diagonal((size[0], size[1], 1.0, 1.0)))
    sheet.matrix_world = matrix
    mod = sheet.modifiers.new('cloth', 'CLOTH')
    st = mod.settings
    st.quality = 8
    st.mass = mass
    st.tension_stiffness = 20.0
    st.compression_stiffness = 20.0
    st.shear_stiffness = 8.0
    st.bending_stiffness = bend
    st.air_damping = 2.0
    cs = mod.collision_settings
    cs.collision_quality = 4
    cs.distance_min = gap
    cs.use_self_collision = self_collide
    cs.self_distance_min = gap
    mod.point_cache.frame_start = 1
    mod.point_cache.frame_end = frames
    sc = bpy.context.scene
    sc.frame_start = 1
    sc.frame_end = frames
    bpy.context.view_layer.update()
    t0 = time.time()
    for f in range(1, frames + 1):
        sc.frame_set(f)
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(sheet.evaluated_get(dg))
    me.transform(sheet.matrix_world)
    out = bpy.data.objects.new(name, me)
    core.link(out)
    for c in cols:
        bpy.data.objects.remove(c)
    bpy.data.objects.remove(sheet)
    sc.frame_set(1)
    if subdiv:
        core.subdivide(out, subdiv)
    out.data.shade_smooth()
    print(f'[dee_room] drape {name}: {len(me.vertices)} verts, {frames} frames in {time.time() - t0:.0f}s')
    return out


def thicken(obj, t, uv_front=(0.0, 0.0, 1.0, 0.8), uv_back=(0.0, 0.82, 1.0, 0.98), uv_rim=(0.0, 0.985, 1.0, 0.995)):
    """Gives a draped sheet thickness t (toward its underside) with its own layout UVs: the sheet's UVs (0..1) scaled
    into uv_front on top, uv_back underneath, the rim in a strip. The sheet UVs go to 'pat' for the pattern. Locks the
    layout."""
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.normal_update()
    uvl = bm.loops.layers.uv.verify()
    sheet_uv = {}
    for f in bm.faces:
        for l in f.loops:
            sheet_uv[l.vert] = l[uvl].uv.copy()
    front_faces = list(bm.faces)
    boundary = [e for e in bm.edges if e.is_boundary]
    back_of = {}
    for v in list(bm.verts):
        back_of[v] = bm.verts.new(v.co - v.normal * t)
    base = {v: v for v in back_of}
    base.update({b: v for v, b in back_of.items()})
    is_back = set(back_of.values())
    back_faces = set()
    for f in front_faces:
        back_faces.add(bm.faces.new([back_of[v] for v in reversed(f.verts)]))
    rim_s = {}
    nb = max(len(boundary), 1)
    for i, e in enumerate(boundary):
        f = e.link_faces[0]
        loop = next(l for l in f.loops if l.edge == e)
        a, b = loop.vert, loop.link_loop_next.vert
        rf = bm.faces.new((b, a, back_of[a], back_of[b]))
        rim_s[rf] = {b: (i + 1) / nb, a: i / nb, back_of[a]: i / nb, back_of[b]: (i + 1) / nb}
    pat = bm.loops.layers.uv.new('pat')

    def put(rect, uv):
        u0, v0, u1, v1 = rect
        return (u0 + (u1 - u0) * uv[0], v0 + (v1 - v0) * uv[1])

    for f in bm.faces:
        for l in f.loops:
            suv = sheet_uv[base[l.vert]]
            l[pat].uv = suv
            if f in rim_s:
                s = rim_s[f][l.vert]
                l[uvl].uv = (uv_rim[0] + (uv_rim[2] - uv_rim[0]) * s, uv_rim[3] if l.vert in is_back else uv_rim[1])
            elif f in back_faces:
                l[uvl].uv = put(uv_back, suv)
            else:
                l[uvl].uv = put(uv_front, suv)
    bm.to_mesh(me)
    bm.free()
    me.uv_layers.active = me.uv_layers['UVMap']
    obj.data.shade_smooth()
    return set_attr(obj, 'dr_uvlock', 1.0)


# ================================================================== materials (shared)

def uv_coords(m, name='pat'):
    n = m.node('ShaderNodeUVMap')
    n.uv_map = name
    return n.outputs['UV']


def attr(m, name):
    n = m.node('ShaderNodeAttribute')
    n.attribute_type = 'GEOMETRY'
    n.attribute_name = name
    return n.outputs['Fac']


def is_k(m, s, k):
    return m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', s, float(k))), 0.5)


def set_bump(m, height, strength, distance):
    b = m.node('ShaderNodeBump', Distance=distance)
    if isinstance(strength, (int, float)):
        b.inputs['Strength'].default_value = strength
    else:
        m.link(strength, b.inputs['Strength'])
    m.link(height, b.inputs['Height'])
    m.set('Normal', b.outputs['Normal'])
    return b


def dist3(m, sep, p):
    dx = m.math('SUBTRACT', sep.outputs['X'], p[0])
    dy = m.math('SUBTRACT', sep.outputs['Y'], p[1])
    dz = m.math('SUBTRACT', sep.outputs['Z'], p[2])
    return m.math('SQRT', m.math('ADD', m.math('ADD', m.math('MULTIPLY', dx, dx), m.math('MULTIPLY', dy, dy)), m.math('MULTIPLY', dz, dz)))


def box_mask(m, sep, lo, hi, soft=0.01):
    """1 inside the axis box lo..hi (None: unbounded on that axis), soft edges."""
    out = None
    for i, ax in enumerate('XYZ'):
        c = sep.outputs[ax]
        terms = []
        if lo[i] is not None:
            terms.append(ramp(m, c, lo[i] - soft, lo[i] + soft))
        if hi[i] is not None:
            terms.append(m.math('SUBTRACT', 1.0, ramp(m, c, hi[i] - soft, hi[i] + soft)))
        for t in terms:
            out = t if out is None else m.math('MULTIPLY', out, t)
    return out if out is not None else 1.0


def up_facing(m, lo=0.55, hi=0.85):
    """0..1 by how much the surface faces up (the bake's object frame): where dust settles."""
    g = m.node('ShaderNodeNewGeometry')
    s = m.node('ShaderNodeSeparateXYZ')
    m.link(g.outputs['Normal'], s.inputs['Vector'])
    return ramp(m, s.outputs['Z'], lo, hi)


def cavity(m, distance=0.02, gain=1.6, samples=12):
    ao = m.node('ShaderNodeAmbientOcclusion', Distance=distance)
    ao.samples = samples
    ao.only_local = True
    return m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ao.outputs['AO']), gain, clamp=True)


def velour(name, base, crush, worn, soil, wear=None, soiled=None, burn=None, dust=None):
    """Crushed velour: the pile's nap pressed every which way (lighter and greyer where it lies flat), flattened and
    shiny where it is sat on (wear), darker where hands rest (soiled); an optional cigarette burn (point, radius)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    swirl = noise(m, o, scale=28.0, detail=4.0, distortion=3.0)
    crushed = ramp(m, swirl, 0.38, 0.66)
    broad = noise(m, o, scale=3.0, detail=2.0)
    fine = noise(m, o, scale=2600.0, detail=1.0)
    nap = noise(m, scaled(m, sep, 900.0, 60.0, 900.0), detail=2.0)
    col = m.mix(m.math('MULTIPLY', crushed, 0.38), H(base), H(crush))
    col = m.mix(m.math('MULTIPLY', ramp(m, broad, 0.35, 0.7), 0.25), col, H(crush))
    col = m.mix(m.math('MULTIPLY', m.math('SUBTRACT', nap, 0.3), 0.15), col, H(crush))
    col = m.mix(m.math('MULTIPLY', fine, 0.18), col, H(base))
    rough = m.math('ADD', 0.88, m.math('MULTIPLY', fine, 0.06))
    rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', crushed, 0.08))
    height = m.math('ADD', m.math('MULTIPLY', fine, 0.5), m.math('MULTIPLY', nap, 0.3))
    if wear is not None:
        w = m.math('MULTIPLY', wear(m, sep), ramp(m, noise(m, o, scale=14.0, detail=4.0), 0.25, 0.6))
        col = m.mix(m.math('MULTIPLY', w, 0.75), col, H(worn))
        rough = mix_float(m, w, rough, 0.74)
        height = mix_float(m, w, height, 0.2)
    if soiled is not None:
        s = m.math('MULTIPLY', soiled(m, sep), ramp(m, noise(m, o, scale=9.0, detail=4.0), 0.3, 0.7))
        col = m.mix(m.math('MULTIPLY', s, 0.6), col, H(soil))
        rough = mix_float(m, s, rough, 0.62)
    if dust is not None:
        d = m.math('MULTIPLY', dust(m, sep), ramp(m, noise(m, o, scale=40.0, detail=3.0), 0.3, 0.8))
        col = m.mix(m.math('MULTIPLY', d, 0.35), col, H(0x8e8579))
    if burn is not None:
        (p, rad) = burn
        d = dist3(m, sep, p)
        wob = m.math('ADD', d, m.math('MULTIPLY', m.math('SUBTRACT', noise(m, o, scale=600.0, detail=3.0), 0.5), rad * 0.5))
        hole = smooth_less(m, wob, rad * 0.45, rad * 0.08)
        ring = m.math('SUBTRACT', smooth_less(m, wob, rad, rad * 0.25), hole)
        col = m.mix(m.math('MULTIPLY', ring, 0.85), col, H(0x2a160c))
        col = m.mix(hole, col, H(0x0c0907))
        rough = mix_float(m, hole, rough, 0.5)
        height = m.math('SUBTRACT', height, m.math('MULTIPLY', hole, 2.0))
    m.set('Base Color', col)
    m.set('Roughness', m.math('MINIMUM', rough, 0.95))
    m.set('Sheen Weight', 0.45)
    m.set('Sheen Roughness', 0.35)
    m.set('Specular IOR Level', 0.16)
    set_bump(m, height, 0.25, 0.0006)
    return m


def wale_pat(obj, rule='generic'):
    """Sets 'pat' u to the coordinate across a corduroy's wales, per face: 'generic' runs the wales up vertical faces
    and front to back on horizontal ones (sides facing X run them up too); 'arm' runs them over a rolled arm (across
    Y) and up its front."""
    me = obj.data
    me.update()
    ensure_uv(obj)
    layer = me.uv_layers.get('pat') or me.uv_layers.new(name='pat')
    co = verts_np(obj)
    npoly = len(me.polygons)
    fn = np.empty(npoly * 3)
    me.polygons.foreach_get('normal', fn)
    fn = fn.reshape(-1, 3)
    tot = np.empty(npoly, dtype=np.int64)
    me.polygons.foreach_get('loop_total', tot)
    face = np.repeat(np.arange(npoly), tot)
    vi = np.empty(len(me.loops), dtype=np.int64)
    me.loops.foreach_get('vertex_index', vi)
    N = fn[face]
    P = co[vi]
    if rule == 'arm':
        u = np.where(N[:, 1] < -0.7, P[:, 0], P[:, 1])
    else:
        u = np.where(np.abs(N[:, 0]) > 0.7, P[:, 1], P[:, 0])
    uv = np.stack([u, np.zeros_like(u)], axis=1).astype(np.float32)
    layer.data.foreach_set('uv', uv.ravel())
    me.uv_layers.active = me.uv_layers['UVMap']
    return obj


def corduroy(name, base, ridge, valley, worn, soil, period=0.0056, wear=None, soiled=None, burn=None, dust=None):
    """Wide-wale corduroy: rounded pile ridges with cut channels between them (pat u runs across the wales; faces
    tagged dr_k 1 are plain, the welts), the ridges rubbed bald and shiny where it is sat on and leaned on (wear),
    darker where hands rest (soiled), dust in the channels on top, an optional cigarette burn (point, radius)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    k = attr(m, 'dr_k')
    plain = is_k(m, k, 1)
    pu = m.node('ShaderNodeSeparateXYZ')
    m.link(uv_coords(m), pu.inputs['Vector'])
    ph = m.math('FRACT', m.math('DIVIDE', pu.outputs['X'], period))
    rid = m.math('POWER', m.math('SINE', m.math('MULTIPLY', ph, math.pi)), 0.55)
    rid = mix_float(m, plain, rid, 0.75)
    pile = noise(m, o, scale=1600.0, detail=2.0)
    fade = noise(m, o, scale=4.0, detail=3.0)
    w = 0.0
    if wear is not None:
        w = m.math('MULTIPLY', wear(m, sep), ramp(m, noise(m, o, scale=16.0, detail=4.0), 0.25, 0.62))
    bald = m.math('MULTIPLY', w, 0.75)
    r_eff = m.math('MULTIPLY', rid, m.math('SUBTRACT', 1.0, m.math('MULTIPLY', bald, 0.6)))
    col = m.mix(r_eff, H(valley), H(ridge))
    col = m.mix(m.math('MULTIPLY', ramp(m, fade, 0.3, 0.75), 0.3), col, H(base))
    col = m.mix(m.math('MULTIPLY', m.math('SUBTRACT', pile, 0.35), 0.35), col, H(valley))
    # bald: the pile gone off the ridges, the ground weave greyer and paler
    col = m.mix(m.math('MULTIPLY', bald, rid), col, H(worn))
    rough = m.math('ADD', 0.9, m.math('MULTIPLY', pile, 0.05))
    rough = mix_float(m, m.math('MULTIPLY', bald, rid), rough, 0.7)
    height = m.math('ADD', m.math('MULTIPLY', r_eff, 1.0), m.math('MULTIPLY', pile, 0.15))
    if soiled is not None:
        s = m.math('MULTIPLY', soiled(m, sep), ramp(m, noise(m, o, scale=9.0, detail=4.0), 0.3, 0.7))
        col = m.mix(m.math('MULTIPLY', s, 0.55), col, H(soil))
        rough = mix_float(m, s, rough, 0.72)
    if dust is not None:
        d = m.math('MULTIPLY', m.math('MULTIPLY', dust(m, sep), m.math('SUBTRACT', 1.0, rid)), ramp(m, noise(m, o, scale=40.0, detail=3.0), 0.3, 0.8))
        col = m.mix(m.math('MULTIPLY', d, 0.5), col, H(0x8e8579))
    if burn is not None:
        (p, rad) = burn
        d = dist3(m, sep, p)
        wob = m.math('ADD', d, m.math('MULTIPLY', m.math('SUBTRACT', noise(m, o, scale=600.0, detail=3.0), 0.5), rad * 0.5))
        hole = smooth_less(m, wob, rad * 0.42, rad * 0.08)
        ring = m.math('SUBTRACT', smooth_less(m, wob, rad, rad * 0.25), hole)
        col = m.mix(m.math('MULTIPLY', ring, 0.85), col, H(0x2a160c))
        col = m.mix(hole, col, H(0x0c0907))
        height = m.math('SUBTRACT', height, m.math('MULTIPLY', hole, 2.0))
    m.set('Base Color', col)
    m.set('Roughness', m.math('MINIMUM', rough, 0.95))
    m.set('Sheen Weight', 0.3)
    m.set('Sheen Roughness', 0.4)
    m.set('Specular IOR Level', 0.18)
    set_bump(m, height, 0.55, 0.0011)
    return m


def wood(name, light, dark, axis='X', ring=160.0, rough=0.38, wear=0.6, grime=0.0, grime_z=None, figure=0.5, center=(0.0, -0.2),
         pores=0.5, extra=None):
    """Stained and lacquered wood, the grain along axis: growth rings around a line parallel to the grain (flat-sawn
    arches on wide faces, rings on end grain), pores, lacquer worn through on convex edges. extra(m, sep, col, rough)
    -> (col, rough) adds a prop's own marks."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    ai = 'XYZ'.index(axis)
    others = [c for c in 'XYZ' if c != axis]
    s = [600.0, 600.0, 600.0]
    s[ai] = 8.0
    a = m.math('SUBTRACT', sep.outputs[others[0]], center[0])
    b = m.math('SUBTRACT', sep.outputs[others[1]], center[1])
    r = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', a, a), m.math('MULTIPLY', b, b)))
    st = [30.0, 30.0, 30.0]
    st[ai] = 2.5
    wob = noise(m, scaled(m, sep, *st), detail=4.0)
    r = m.math('ADD', r, m.math('MULTIPLY', m.math('SUBTRACT', wob, 0.5), 0.02))
    rings = m.math('FRACT', m.math('MULTIPLY', r, ring))
    late = m.math('MULTIPLY', ramp(m, rings, 0.55, 0.85), m.math('SUBTRACT', 1.0, ramp(m, rings, 0.92, 1.0)))
    sf = [6.0, 6.0, 6.0]
    sf[ai] = 0.8
    fig = noise(m, scaled(m, sep, *sf), detail=3.0)
    col = m.mix(m.math('MULTIPLY', ramp(m, fig, 0.3, 0.75), figure), H(light), H(dark))
    col = m.mix(m.math('MULTIPLY', late, 0.6), col, H(dark))
    pr = noise(m, scaled(m, sep, *s), detail=2.0)
    pore = ramp(m, pr, 0.62, 0.74)
    col = m.mix(m.math('MULTIPLY', pore, pores * 0.5), col, H(dark))
    rgh = m.math('ADD', rough, m.math('MULTIPLY', noise(m, o, scale=8.0, detail=3.0), 0.06))
    if wear > 0:
        e = m.math('MULTIPLY', convex_edges(m, tc, distance=0.003, samples=12, breakup_scale=60.0, gain=4.0), wear)
        e = ramp(m, e, 0.15, 0.6)
        col = m.mix(m.math('MULTIPLY', e, 0.7), col, H(_lighten(light, 0.35)))
        rgh = mix_float(m, e, rgh, 0.62)
    if grime > 0:
        c = cavity(m, 0.015, 1.4)
        g = m.math('MULTIPLY', c, grime)
        if grime_z is not None:
            g = m.math('MAXIMUM', g, m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ramp(m, sep.outputs['Z'], grime_z[0], grime_z[1])), grime * 0.6))
        col = m.mix(m.math('MULTIPLY', g, 0.7), col, H(0x1e140c))
        rgh = mix_float(m, g, rgh, 0.75)
    if extra is not None:
        col, rgh = extra(m, sep, tc, col, rgh)
    m.set('Base Color', col)
    m.set('Roughness', m.math('MINIMUM', rgh, 0.95))
    set_bump(m, m.math('SUBTRACT', m.math('MULTIPLY', late, 0.3), m.math('MULTIPLY', pore, 0.5)), 0.15, 0.0002)
    return m


def _lighten(h, amount):
    r, g, b = (h >> 16) & 255, (h >> 8) & 255, h & 255
    r, g, b = (int(c + (255 - c) * amount) for c in (r, g, b))
    return (r << 16) | (g << 8) | b


def _shade(h, f):
    r, g, b = (h >> 16) & 255, (h >> 8) & 255, h & 255
    r, g, b = (max(0, min(255, int(c * f))) for c in (r, g, b))
    return (r << 16) | (g << 8) | b


# ================================================================== pattern images (numpy)

def _vn2(X, Y, scale, seed=0, octaves=3):
    P = np.stack([X.ravel(), Y.ravel(), np.zeros(X.size)], axis=1)
    return texmaps.value_noise(P, scale, seed=seed, octaves=octaves).reshape(X.shape)


def _hash(i, j, seed=0):
    h = (np.asarray(i, np.int64) * 73856093) ^ (np.asarray(j, np.int64) * 19349663) ^ (seed * 83492791)
    h = (h ^ (h >> 13)) * 1274126177
    h = h ^ (h >> 16)
    return (h & 0xFFFFFF).astype(np.float64) / float(0xFFFFFF)


def _lin(h):
    return np.array(core.hex_linear(h)[:3])


def save_img(name, rgb, colorspace='sRGB'):
    """rgb rows top-down (image convention) -> PNG in the build textures; returns its path."""
    return texmaps.save(name, np.ascontiguousarray(rgb[::-1]), colorspace)


def granny_throw(px=2048, sq=(9, 11), side=0.138, border=0.022, seed=5):
    """The throw's crochet: granny squares (rounds of three-stitch clusters with chain-space holes between them),
    joined and bordered in black. Returns (color path, height path, sheet size in meters)."""
    Wm, Hm = sq[0] * side + 2 * border, sq[1] * side + 2 * border
    W = px
    Hh = int(round(px * Hm / Wm))
    xs = (np.arange(W) + 0.5) / W * Wm
    ys = (np.arange(Hh) + 0.5) / Hh * Hm
    X, Y = np.meshgrid(xs, ys[::-1])  # rows top-down: the top row is the far end (v = 1)
    palette = [0xc8962a, 0xb4542a, 0x2f7a78, 0xe3d7bd, 0x8c2f2c, 0x6f7a35, 0xc98a86, 0x5b3a26, 0x3d5a8a]
    pal = np.array([_lin(c) for c in palette])
    black = _lin(0x16120f)
    hole_c = _lin(0x2a120c)
    gx = (X - border) / side
    gy = (Y - border) / side
    inside = (gx >= 0) & (gx < sq[0]) & (gy >= 0) & (gy < sq[1])
    ix = np.clip(np.floor(gx), 0, sq[0] - 1)
    iy = np.clip(np.floor(gy), 0, sq[1] - 1)
    lx = (gx - ix) * 2 - 1
    ly = (gy - iy) * 2 - 1
    c = np.maximum(np.abs(lx), np.abs(ly))
    NR = 5
    k = np.minimum(np.floor(c * NR), NR - 1)
    fr = c * NR - k
    horiz = np.abs(ly) >= np.abs(lx)
    a = np.where(horiz, lx, ly) / np.maximum(c, 1e-6)  # -1..1 along the side
    mcl = k + 1  # clusters per side
    pos = (a + 1) / 2 * mcl
    jc = np.floor(pos)
    off = pos - jc - 0.5  # -0.5..0.5 from a cluster's middle
    gap = np.minimum(np.abs(pos - np.round(pos)), 1.0) * c * NR / NR  # distance to a hole along the side
    # chain-space holes between clusters and at the corners, in the round's base
    hole_w = 0.16
    hole = (np.abs(pos - np.round(pos)) < hole_w / np.maximum(mcl, 1) * 1.2 + 0.06) & (fr > 0.12) & (fr < 0.72) & (k >= 1)
    hole |= (c < 0.07)
    # stitches: three posts per cluster running out from the middle, plies twisting across them
    posts = 0.5 + 0.5 * np.cos(off * 2 * np.pi * 3)
    plies = 0.5 + 0.5 * np.sin((fr * 9.0 + off * 4.0) * 2 * np.pi)
    hgt = 0.55 + 0.25 * posts + 0.12 * plies
    hgt *= np.clip(np.sin(np.clip(fr, 0, 1) * np.pi) * 1.4, 0.35, 1.0)
    # colors: a different yarn per round of each square, the last round black
    sq_id = (ix * 31 + iy * 17).astype(np.int64)
    choice = np.floor(_hash(sq_id, k.astype(np.int64), seed) * len(palette)).astype(np.int64)
    # avoid the same color in successive rounds
    prev = np.floor(_hash(sq_id, (k - 1).astype(np.int64), seed) * len(palette)).astype(np.int64)
    choice = np.where(choice == prev, (choice + 3) % len(palette), choice)
    col = pal[choice]
    col = np.where((k >= NR - 1)[..., None], black, col)
    # joins: a slip-stitch ridge between squares
    join = c > 0.965
    col = np.where(join[..., None], black * 0.8, col)
    hgt = np.where(join, 0.75 + 0.2 * np.cos(off * 2 * np.pi * 6), hgt)
    # the border around the whole throw
    bd = ~inside
    bdist = np.minimum.reduce([X, Wm - X, Y, Hm - Y])
    brow = 0.5 + 0.5 * np.cos(bdist / 0.0085 * 2 * np.pi)
    bcol = 0.5 + 0.5 * np.cos((X + Y) / 0.006 * 2 * np.pi)
    col = np.where(bd[..., None], black, col)
    hgt = np.where(bd, 0.6 + 0.25 * brow * (0.6 + 0.4 * bcol), hgt)
    # yarn variation and age: a little fading and fuzz
    fz = _vn2(X, Y, 0.004, seed=11, octaves=2)
    fade = _vn2(X, Y, 0.25, seed=12, octaves=3)
    col = col * (0.82 + 0.3 * fz[..., None]) * (0.9 + 0.12 * fade[..., None])
    col = col * (0.72 + 0.28 * hgt[..., None])
    col = np.where(hole[..., None] & inside[..., None], hole_c, col)
    hgt = np.where(hole & inside, 0.0, hgt)
    hgt = hgt * (0.85 + 0.15 * fz)
    cp = save_img('dr_throw_color', np.clip(col, 0, 1))
    hp = save_img('dr_throw_height', np.repeat(np.clip(hgt, 0, 1)[..., None], 3, axis=2), 'Non-Color')
    return cp, hp, (Wm, Hm)


def floral_tapestry(px=1024, seed=3):
    """A woven floral for Dee's pillow: cabbage roses and leaves on a cream ground, the weave showing."""
    W = Hh = px
    xs = (np.arange(W) + 0.5) / W
    X, Y = np.meshgrid(xs, xs[::-1])
    rng = random.Random(seed)
    col = np.tile(_lin(0xc4ae86), (Hh, W, 1))
    ground2 = _lin(0xa8916a)
    col = col * 0.9 + ground2 * 0.1 * _vn2(X, Y, 0.05, 2)[..., None]
    hgt = np.full((Hh, W), 0.5)

    def blend(mask, c):
        nonlocal col
        col = col * (1 - mask[..., None]) + np.asarray(c) * mask[..., None]

    # leaves first, then roses over them, then small blue flowers
    centers = []
    for gyi in range(3):
        for gxi in range(3):
            cx = (gxi + 0.5 + (0.5 if gyi % 2 else 0.0)) / 3 % 1.0 + rng.uniform(-0.05, 0.05)
            cy = (gyi + 0.5) / 3 + rng.uniform(-0.05, 0.05)
            centers.append((cx, cy, rng.uniform(0.075, 0.1)))
    for cx, cy, R in centers:
        for k in range(5):
            ang = rng.uniform(0, 2 * math.pi)
            L = R * rng.uniform(1.3, 1.9)
            lx, ly = cx + math.cos(ang) * L * 0.75, cy + math.sin(ang) * L * 0.75
            for ox in (-1, 0, 1):
                dx = X - (lx + ox)
                dy = Y - ly
                u = dx * math.cos(ang) + dy * math.sin(ang)
                v = -dx * math.sin(ang) + dy * math.cos(ang)
                leaf = ((u / (L * 0.55)) ** 2 + (v / (L * 0.2)) ** 2) < 1.0
                lm = leaf.astype(float)
                shade = 0.6 + 0.4 * (v > 0)
                rib = np.exp(-(v / 0.004) ** 2) * leaf
                blend(lm, _lin(0x505a36))
                blend(lm * (v > 0) * 0.35, _lin(0x747e52))
                blend(rib * 0.6, _lin(0x3a4524))
    for cx, cy, R in centers:
        for ox in (-1, 0, 1):
            dx = X - (cx + ox)
            dy = Y - cy
            d = np.sqrt(dx * dx + dy * dy)
            th = np.arctan2(dy, dx)
            edge = R * (1 + 0.08 * np.sin(th * 5 + cx * 10))
            rose = d < edge
            rho = d / R
            petals = 0.5 + 0.5 * np.sin(th * 3 + rho * 14)
            tone = np.clip(0.35 + 0.55 * rho + 0.2 * petals, 0, 1)
            c = _lin(0x48121c)[None, None] * (1 - tone[..., None]) + _lin(0x96464c)[None, None] * tone[..., None]
            lines = np.exp(-((np.sin(rho * 12 + th * 1.5)) / 0.25) ** 2) * rose
            m_ = rose.astype(float)
            col = col * (1 - m_[..., None]) + c * m_[..., None]
            blend(lines * 0.5, _lin(0x4a0c16))
    for k in range(14):
        cx, cy = rng.random(), rng.random()
        for ox in (-1, 0, 1):
            d = np.sqrt((X - cx - ox) ** 2 + (Y - cy) ** 2)
            th = np.arctan2(Y - cy, X - cx - ox)
            r = 0.022 * (0.7 + 0.3 * np.cos(th * 5))
            blend((d < r).astype(float), _lin(0x46566e))
            blend((d < 0.006).astype(float), _lin(0xd8b44a))
    # the weave: warp and weft about a millimeter apart
    n = px / 0.46 * 0.0011
    wx = 0.5 + 0.5 * np.cos(X * px / n * 2 * np.pi)
    wy = 0.5 + 0.5 * np.cos(Y * px / n * 2 * np.pi)
    weave = np.where(((np.floor(X * px / n) + np.floor(Y * px / n)) % 2) == 0, wx, wy)
    col = col * (0.82 + 0.25 * weave[..., None])
    hgt = 0.3 + 0.6 * weave
    fade = _vn2(X, Y, 0.3, 21)
    col = col * (0.85 + 0.2 * fade[..., None])
    cp = save_img('dr_floral_color', np.clip(col, 0, 1))
    hp = save_img('dr_floral_height', np.repeat(np.clip(hgt, 0, 1)[..., None], 3, axis=2), 'Non-Color')
    return cp, hp


def doily_edge(th):
    """The doily's scalloped rim (fraction of its radius) at angle th: 24 round scallops."""
    return 0.93 + 0.07 * np.abs(np.cos(12 * np.asarray(th))) ** 0.6


def doily_img(px=900):
    """A crocheted pineapple doily from above (numpy, linear, rows top-down) filling the square, its rim at
    doily_edge: a solid rosette, a ring of chain holes, eight pineapples in a net of chain loops, a band, arches, a
    fine mesh and the scallops. Ivory cotton gone a little yellow toward the rim; the holes show the TV's dark
    plastic. The middle is solid (the panel's sides sample it)."""
    c = (np.arange(px) + 0.5) / px * 2 - 1
    X, Y = np.meshgrid(c, -c)
    r = np.hypot(X, Y)
    th = np.arctan2(Y, X)
    tau = 2 * np.pi
    edge = doily_edge(th)
    holes = np.zeros(r.shape, bool)
    # rosette petals (no holes), a ring of 16 chain spaces
    ph16 = (th / tau * 16) % 1 - 0.5
    holes |= (np.abs(ph16) < 0.2) & (r > 0.13) & (r < 0.17)
    # eight pineapples, fat in the middle, in a net of chain loops; V-stitches along their sides
    ph8 = (th / tau * 8 + 0.5) % 1 - 0.5
    band = (r > 0.2) & (r < 0.56)
    t = np.clip((r - 0.2) / 0.36, 0, 1)
    pw = 0.34 * np.sin(np.pi * (0.12 + 0.88 * t)) ** 0.55
    pine = band & (np.abs(ph8) < pw)
    rowi = np.floor(r * 26)
    nu = (r * 26) % 1 - 0.5
    nv = (ph8 * 7 + (rowi % 2) * 0.5) % 1 - 0.5
    holes |= band & ~pine & ((np.abs(nu) / 0.38 + np.abs(nv) / 0.42) < 1)
    holes |= band & (np.abs(np.abs(ph8) - pw) < 0.022) & ((r * 70) % 1 < 0.45)
    # a solid band with a ring of small holes, then 24 arches
    ph48 = (th / tau * 48) % 1 - 0.5
    holes |= (np.abs(r - 0.595) < 0.011) & (np.abs(ph48) < 0.26)
    ph24 = (th / tau * 24 + 0.5) % 1 - 0.5
    holes |= (((ph24 / 0.4) ** 2 + ((r - 0.635) / 0.2) ** 2) < 1) & (r > 0.645)
    # the fine mesh before the scallops, and a picot hole in each scallop
    ph72 = (th / tau * 72) % 1 - 0.5
    holes |= (r > 0.85) & (r < edge - 0.035) & (np.abs(ph72) < 0.27) & (np.abs(((r - 0.85) * 36) % 1 - 0.5) < 0.3)
    holes |= (np.hypot(ph24 * tau / 24 * 0.97, r - 0.955) < 0.012)
    outside = r > edge
    # thread: rows round the doily and posts across them, a little fuzz; yellowed toward the rim
    rows = 0.5 + 0.5 * np.cos(r * tau * 52)
    posts = 0.5 + 0.5 * np.cos(th * 300 + r * 40)
    fuzz = _vn2(X, Y, 0.012, seed=31, octaves=2)
    shade = 0.8 + 0.09 * rows + 0.07 * posts + 0.08 * fuzz
    ivory = _lin(0xebe3d0) * (1 - 0.3 * np.clip(r - 0.4, 0, 1)[..., None]) + _lin(0xcdbd98) * 0.3 * np.clip(r - 0.4, 0, 1)[..., None]
    thread = ivory * shade[..., None]
    # an old ring where a glass once stood
    ring_ = np.exp(-((np.hypot(X - 0.32, Y + 0.22) - 0.19) / 0.012) ** 2) * (0.5 + 0.5 * _vn2(X, Y, 0.08, seed=33))
    thread = thread * (1 - 0.25 * ring_[..., None]) + _lin(0x9a7a4a) * 0.25 * ring_[..., None]
    solid = blur((~holes & ~outside).astype(float), 1)
    ao = blur(solid, 3)
    thread = thread * (0.7 + 0.3 * ao[..., None])
    dark = _lin(0x262729)
    col = thread * solid[..., None] + dark * (1 - solid[..., None])
    return np.clip(col, 0, 1)


def doily_disc(name, atlas, key, radius, z0, z1, nr=9, na=120):
    """The doily as a thin disc from z0 to z1 with rings of vertices inside it (so it can follow a curved top), its
    rim scalloped (doily_edge); top and bottom printed with atlas[key] (the image fills the square round the rim), the
    rim's edge sampling the middle."""
    bm = bmesh.new()
    ths = [2 * math.pi * i / na for i in range(na)]

    def ring(z, k):
        return [bm.verts.new((radius * float(doily_edge(t)) * k / nr * math.cos(t), radius * float(doily_edge(t)) * k / nr * math.sin(t), z))
                for t in ths]
    ct, cb = bm.verts.new((0.0, 0.0, z1)), bm.verts.new((0.0, 0.0, z0))
    top = [ring(z1, k) for k in range(1, nr + 1)]
    bot = [ring(z0, k) for k in range(1, nr + 1)]
    for i in range(na):
        j = (i + 1) % na
        bm.faces.new((ct, top[0][i], top[0][j]))
        bm.faces.new((cb, bot[0][j], bot[0][i]))
        for k in range(nr - 1):
            bm.faces.new((top[k][i], top[k + 1][i], top[k + 1][j], top[k][j]))
            bm.faces.new((bot[k][i], bot[k][j], bot[k + 1][j], bot[k + 1][i]))
        bm.faces.new((top[-1][i], bot[-1][i], bot[-1][j], top[-1][j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    obj = core.mesh_object(name, bm)

    def fn(P, N):
        uv = atlas.uv(key, P[:, 0] / (2 * radius) + 0.5, P[:, 1] / (2 * radius) + 0.5)
        sd = np.abs(N[:, 2]) <= 0.5
        uv[sd] = atlas.uv(key, np.full(int(sd.sum()), 0.5), np.full(int(sd.sum()), 0.5))
        return uv
    pat_faces(obj, fn)
    return obj


# ================================================================== prints

# The print axes (u right, v up) of each face of a box, as a viewer outside it reads them.
PRINT_AXES = {'+x': ((0, 1, 0), (0, 0, 1)), '-x': ((0, -1, 0), (0, 0, 1)), '+y': ((-1, 0, 0), (0, 0, 1)),
              '-y': ((1, 0, 0), (0, 0, 1)), '+z': ((1, 0, 0), (0, 1, 0)), '-z': ((1, 0, 0), (0, -1, 0))}


def _srgb(x):
    x = np.clip(x, 0.0, 1.0)
    return np.where(x <= 0.0031308, x * 12.92, 1.055 * np.power(np.maximum(x, 0.0031308), 1 / 2.4) - 0.055)


def _resize(a, h, w):
    """Bilinear resize of an (H, W, C) array."""
    H0, W0 = a.shape[:2]
    ys = np.clip((np.arange(h) + 0.5) / h * H0 - 0.5, 0, H0 - 1)
    xs = np.clip((np.arange(w) + 0.5) / w * W0 - 0.5, 0, W0 - 1)
    y0 = np.floor(ys).astype(int)
    x0 = np.floor(xs).astype(int)
    y1 = np.minimum(y0 + 1, H0 - 1)
    x1 = np.minimum(x0 + 1, W0 - 1)
    fy = (ys - y0)[:, None, None]
    fx = (xs - x0)[None, :, None]
    top = a[y0][:, x0] * (1 - fx) + a[y0][:, x1] * fx
    bot = a[y1][:, x0] * (1 - fx) + a[y1][:, x1] * fx
    return top * (1 - fy) + bot * fy


class Atlas:
    """The prints of one mesh laid out on one Sheet (millimeters); faces sample it through their 'pat' UVs.
    Photographs (numpy, linear, rows top-down) are pasted into their regions after the render."""

    def __init__(self, name, w, h, px, bg=0x7f7f7f):
        self.name, self.px = name, px
        self.s = Sheet(w, h)
        self.s.rect(0, 0, w, h, bg, rough=0.6)
        self.x = self.y = self.row = 0.0
        self.regions = {}
        self.photos = []
        self.paths = None

    def region(self, key, w, h, gap=2.5):
        S = self.s
        if self.x + w > S.w:
            self.x = 0.0
            self.y += self.row + gap
            self.row = 0.0
        if self.y + h > S.h or w > S.w:
            raise RuntimeError(f'atlas {self.name} is full at {key}')
        r = (self.x, self.y, w, h)
        self.x += w + gap
        self.row = max(self.row, h)
        self.regions[key] = r
        return r

    def fill(self, key, w, h, color, rough=0.6, metal=0.0):
        x, y, w, h = self.region(key, w, h)
        self.s.rect(x, y, w, h, color, metal=metal, rough=rough)
        return x, y, w, h

    def uv(self, key, s, t):
        x, y, w, h = self.regions[key]
        return np.stack([(x + np.asarray(s) * w) / self.s.w, (y + np.asarray(t) * h) / self.s.h], axis=-1)

    def photo(self, key, rgb, rough=0.3):
        self.paste(self.regions[key], rgb, rough)

    def paste(self, rect, rgb, rough=0.3):
        x, y, w, h = rect
        self.s.rect(x, y, w, h, 0x808080, rough=rough)
        self.photos.append((rect, rgb))

    def render(self):
        col, surf = self.s.render(self.name, self.px)
        if self.photos:
            img = bpy.data.images.load(col, check_existing=False)
            W, Hh = img.size
            a = np.empty(W * Hh * 4, np.float32)
            img.pixels.foreach_get(a)
            a = a.reshape(Hh, W, 4)
            for (x, y, w, h), rgb in self.photos:
                x0, x1 = int(round(x / self.s.w * W)), int(round((x + w) / self.s.w * W))
                y0, y1 = int(round(y / self.s.h * Hh)), int(round((y + h) / self.s.h * Hh))
                a[y0:y1, x0:x1, :3] = _srgb(_resize(np.asarray(rgb, np.float32), y1 - y0, x1 - x0)[::-1])
            img.pixels.foreach_set(a.ravel())
            img.filepath_raw = col
            img.file_format = 'PNG'
            img.save()
            bpy.data.images.remove(img)
        for obj, *_ in self.s.items:
            bpy.data.objects.remove(obj)
        for o in [o for o in bpy.data.objects if o.name.startswith('sheet_cam')]:
            bpy.data.objects.remove(o)
        self.paths = (col, surf)
        return self.paths


def pat_faces(obj, fn):
    """Sets 'pat' per loop from fn(P, N) -> (L, 2): P the loop's vertex positions, N its face's normal."""
    me = obj.data
    me.update()
    ensure_uv(obj)
    layer = me.uv_layers.get('pat') or me.uv_layers.new(name='pat')
    co = verts_np(obj)
    npoly = len(me.polygons)
    fn_ = np.empty(npoly * 3)
    me.polygons.foreach_get('normal', fn_)
    tot = np.empty(npoly, dtype=np.int64)
    me.polygons.foreach_get('loop_total', tot)
    face = np.repeat(np.arange(npoly), tot)
    vi = np.empty(len(me.loops), dtype=np.int64)
    me.loops.foreach_get('vertex_index', vi)
    uv = np.asarray(fn(co[vi], fn_.reshape(-1, 3)[face]), dtype=np.float32)
    layer.data.foreach_set('uv', uv.ravel())
    me.uv_layers.active = me.uv_layers['UVMap']
    return obj


def box_pat(atlas, size, faces, default=None, center=(0.0, 0.0, 0.0)):
    """A pat_faces function mapping each face of a box (size, centered) onto its atlas region: faces = {'+z': key,
    ...}; other faces take default. Positions are the box's own (before it is moved)."""
    half = np.array(size, float) / 2
    c = np.array(center, float)

    def fn(P, N):
        P = P - c
        uv = np.zeros((len(P), 2))
        done = np.zeros(len(P), bool)
        for key, (ua, va) in PRINT_AXES.items():
            ax = 'xyz'.index(key[1])
            sgn = 1.0 if key[0] == '+' else -1.0
            sel = (N[:, ax] * sgn > 0.6) & ~done
            reg = faces.get(key, default)
            if reg is None or not sel.any():
                continue
            ua, va = np.array(ua, float), np.array(va, float)
            hu = abs(half @ np.abs(ua))
            hv = abs(half @ np.abs(va))
            s = (P[sel] @ ua + hu) / (2 * hu)
            t = (P[sel] @ va + hv) / (2 * hv)
            uv[sel] = atlas.uv(reg, s, t)
            done |= sel
        if default is not None and (~done).any():
            n = int((~done).sum())
            uv[~done] = atlas.uv(default, np.full(n, 0.5), np.full(n, 0.5))
        return uv
    return fn


def print_box(name, size, atlas, faces, default=None, r=0.0, seg=2, matrix=None):
    """A printed box (a card, a deck, a magazine, a tape): its faces mapped onto atlas regions, then moved."""
    o = parts.rbox(name, (0, 0, 0), size, r, seg)
    pat_faces(o, box_pat(atlas, size, faces, default))
    if matrix is not None:
        o.data.transform(matrix)
    return o


def print_panel(name, outline, z0, z1, atlas, top, bottom=None, side=None, r=0.0, matrix=None):
    """A flat shape (a card with round corners, a plaque plate) printed on top (and bottom), extents from outline."""
    o = parts.panel(name, outline, z0, z1, r)
    xs = [p[0] for p in outline]
    ys = [p[1] for p in outline]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)

    def fn(P, N):
        s = (P[:, 0] - x0) / (x1 - x0)
        t = (P[:, 1] - y0) / (y1 - y0)
        uv = atlas.uv(top, s, t)
        if bottom is not None:
            b = N[:, 2] < -0.5
            uv[b] = atlas.uv(bottom, s[b], 1 - t[b])
        if side is not None:
            sd = np.abs(N[:, 2]) <= 0.5
            uv[sd] = atlas.uv(side, np.full(sd.sum(), 0.5), np.full(sd.sum(), 0.5))
        return uv
    pat_faces(o, fn)
    if matrix is not None:
        o.data.transform(matrix)
    return o


def print_material(name, atlas, rough_scale=1.0, bump=0.0):
    """Samples an atlas (color and surface) at 'pat'."""
    m = core.Mat(name)
    col, alpha, metal, rough = image_surface(m, atlas.paths, uv_coords(m), extension='EXTEND')
    tc, sep = object_coords(m)
    grain = noise(m, tc.outputs['Object'], scale=900.0, detail=2.0)
    m.set('Base Color', col)
    m.set('Metallic', metal)
    m.set('Roughness', m.math('MULTIPLY', m.math('ADD', rough, m.math('MULTIPLY', grain, 0.04)), rough_scale))
    m.set('Specular IOR Level', 0.4)
    if bump > 0:
        set_bump(m, grain, bump, 0.0002)
    return m


# ---- drawing helpers (Sheet units = millimeters)

def suit(s, kind_, cx, cy, size, color):
    """A suit symbol centered at (cx, cy), size tall."""
    r = size * 0.27
    if kind_ in 'HS':
        sg = 1 if kind_ == 'H' else -1
        top = cy + sg * size * 0.12
        s.circle(cx - r * 0.98, top, r, color, n=32)
        s.circle(cx + r * 0.98, top, r, color, n=32)
        if sg > 0:
            s.poly([(cx - r * 1.93, top - r * 0.25), (cx + r * 1.93, top - r * 0.25), (cx, cy - size * 0.5)], color)
        else:
            s.poly([(cx + r * 1.93, top + r * 0.25), (cx - r * 1.93, top + r * 0.25), (cx, cy + size * 0.5)], color)
            s.poly([(cx, cy - size * 0.05), (cx - size * 0.2, cy - size * 0.5), (cx + size * 0.2, cy - size * 0.5)], color)
    elif kind_ == 'D':
        s.poly([(cx, cy + size * 0.5), (cx - size * 0.36, cy), (cx, cy - size * 0.5), (cx + size * 0.36, cy)], color)
    else:
        rc = size * 0.235
        s.circle(cx, cy + size * 0.2, rc, color, n=32)
        s.circle(cx - size * 0.235, cy - size * 0.1, rc, color, n=32)
        s.circle(cx + size * 0.235, cy - size * 0.1, rc, color, n=32)
        s.poly([(cx, cy + size * 0.05), (cx - size * 0.18, cy - size * 0.5), (cx + size * 0.18, cy - size * 0.5)], color)


SUIT_COLOR = {'S': 0x161616, 'C': 0x161616, 'H': 0xb3141c, 'D': 0xb3141c}
PIPS = {1: [(0.5, 0.5)], 2: [(0.5, 0.2), (0.5, 0.8)], 3: [(0.5, 0.2), (0.5, 0.5), (0.5, 0.8)],
        4: [(0.3, 0.2), (0.7, 0.2), (0.3, 0.8), (0.7, 0.8)], 5: [(0.3, 0.2), (0.7, 0.2), (0.5, 0.5), (0.3, 0.8), (0.7, 0.8)],
        6: [(0.3, 0.2), (0.7, 0.2), (0.3, 0.5), (0.7, 0.5), (0.3, 0.8), (0.7, 0.8)],
        7: [(0.3, 0.2), (0.7, 0.2), (0.3, 0.5), (0.7, 0.5), (0.5, 0.35), (0.3, 0.8), (0.7, 0.8)],
        8: [(0.3, 0.2), (0.7, 0.2), (0.3, 0.5), (0.7, 0.5), (0.5, 0.35), (0.5, 0.65), (0.3, 0.8), (0.7, 0.8)],
        9: [(0.3, 0.2), (0.7, 0.2), (0.3, 0.4), (0.7, 0.4), (0.3, 0.6), (0.7, 0.6), (0.3, 0.8), (0.7, 0.8), (0.5, 0.5)],
        10: [(0.3, 0.2), (0.7, 0.2), (0.3, 0.4), (0.7, 0.4), (0.3, 0.6), (0.7, 0.6), (0.3, 0.8), (0.7, 0.8), (0.5, 0.3), (0.5, 0.7)]}


def card_face(s, x, y, rank, st, w=63.5, h=88.9):
    """A playing card's face in the rectangle at (x, y)."""
    c = SUIT_COLOR[st]
    s.rect(x, y, w, h, 0xf4f1ea, rough=0.35)
    label = {1: 'A', 11: 'J', 12: 'Q', 13: 'K'}.get(rank, str(rank))
    s.text(label, x + 6.5, y + h - 11.5, 9.5, c, face='Bold', align='CENTER', rough=0.35)
    suit(s, st, x + 6.5, y + h - 17.5, 5.5, c)
    s.text(label, x + w - 6.5, y + 11.5, 9.5, c, face='Bold', align='CENTER', rot=math.pi, rough=0.35)
    suit(s, st, x + w - 6.5, y + 17.5, 5.5, c)
    if rank > 10:
        s.rect(x + 12, y + 12, w - 24, h - 24, 0xc8a02a, rough=0.35)
        s.rect(x + 12.6, y + 12.6, w - 25.2, h - 25.2, 0xf4f1ea, rough=0.35)
        for cc, yy in ((0xc8a02a, 0.72), (0xb3141c, 0.55), (0x1c3a8a, 0.4), (0xc8a02a, 0.27)):
            s.rect(x + 16, y + 14 + (h - 28) * yy - 5, w - 32, 10, cc, rough=0.35)
        s.text(label, x + w / 2, y + h / 2 - 8, 22, c, face='Black', align='CENTER', rough=0.35)
    else:
        for px_, py_ in PIPS[rank]:
            big = 22 if rank == 1 else 12
            suit(s, st, x + 12 + (w - 24) * px_, y + 14 + (h - 28) * py_, big, c)


def card_back(s, x, y, w=63.5, h=88.9, color=0x9e1b25):
    """A card's back: a white margin, a red field with a diamond lattice, a ring in the middle."""
    s.rect(x, y, w, h, 0xf4f1ea, rough=0.35)
    s.rect(x + 3.5, y + 3.5, w - 7, h - 7, color, rough=0.32)
    step = 5.0
    fw, fh = w - 9, h - 9
    for i in range(int(fw / step) + 1):
        for j in range(int(fh / step) + 1):
            cx, cy = x + 4.5 + i * step + step / 2, y + 4.5 + j * step + step / 2
            if cx > x + w - 5 or cy > y + h - 5:
                continue
            s.poly([(cx, cy + 1.6), (cx - 1.6, cy), (cx, cy - 1.6), (cx + 1.6, cy)], 0xe9c8c0, rough=0.32)
            s.poly([(cx, cy + 0.8), (cx - 0.8, cy), (cx, cy - 0.8), (cx + 0.8, cy)], color, rough=0.32)
    s.circle(x + w / 2, y + h / 2, 11, color, rough=0.32)
    s.ring(x + w / 2, y + h / 2, 9, 10.5, 0xf1d9d2, rough=0.32)
    s.ring(x + w / 2, y + h / 2, 5, 6, 0xf1d9d2, rough=0.32)


def blur(a, passes=2):
    for _ in range(passes):
        a = (np.roll(a, 1, 0) + np.roll(a, -1, 0) + np.roll(a, 1, 1) + np.roll(a, -1, 1) + 2 * a) / 6
    return a


def soften(a, passes=2):
    """blur() without wrapping round the edges."""
    for _ in range(passes):
        p = np.pad(a, [(1, 1), (1, 1)] + [(0, 0)] * (a.ndim - 2), mode='edge')
        a = (p[:-2, 1:-1] + p[2:, 1:-1] + p[1:-1, :-2] + p[1:-1, 2:] + 2 * a) / 6
    return a


def photo(kind_, w=240, h=160, seed=0):
    """A snapshot (numpy, linear, rows top-down). People are shaded round as a flash lights them: faces with eyes,
    brows, a nose's shadow and a mouth, hair with a sheen, necks, shoulders and arms, clothes with their folds;
    backgrounds go soft with out-of-focus lights. Then the camera and the years: softness, the flash's falloff,
    grain, lifted blacks and a warm cast. Kinds: 'dealer' (young Dee dealing at the Riverside), 'beach', 'school',
    'party', 'wedding', 'lighthouse', 'cover_cards', 'cover_food', 'cover_tv', 'cover_home'."""
    rng = np.random.default_rng(seed)
    ys, xs = np.mgrid[0:h, 0:w]
    u = (xs + 0.5) / w
    v = 1 - (ys + 0.5) / h  # 0 bottom, 1 top
    img = np.zeros((h, w, 3))
    asp = h / w
    sp = max(1, int(round(max(w, h) / 160)))  # blur passes per 'soft' step at this size

    def ell(cx, cy, rx, ry):
        return (((u - cx) / rx) ** 2 + ((v - cy) / ry) ** 2) < 1.0

    def paint(mask, col, soft=1, alpha=1.0):
        """Lays col (a hex, an rgb or an image) where mask is, its edge softened."""
        nonlocal img
        m = soften(np.asarray(mask, float), soft * sp) * alpha if soft else np.asarray(mask, float) * alpha
        c_ = np.asarray(_lin(col) if isinstance(col, int) else col, float)
        if c_.ndim == 1:
            c_ = c_[None, None]
        img = img * (1 - m[..., None]) + c_ * m[..., None]

    def lit(cx, cy, rx, ry):
        """Round shading over an ellipse, the flash a little above and left of the lens."""
        X_, Y_ = (u - cx) / rx, (v - cy) / ry
        nz = np.sqrt(np.clip(1 - X_ ** 2 - Y_ ** 2, 0, 1))
        return np.clip(0.5 + 0.5 * nz - 0.08 * X_ + 0.07 * Y_, 0.35, 1.1)

    def person(cx, base, height, shirt, skin=0xc89070, hair=0x3a2418, width=1.0, outfit=None, long_hair=False, smile=True,
               legs=0x2a2e3a, shorts=0x3a4a6a, full=False):
        H_ = height
        hx, hy = 0.074 * H_ * asp, 0.1 * H_
        hc = base + 0.83 * H_
        sw = 0.2 * width * H_ * asp
        top = base + 0.71 * H_
        # long hair falls behind the shoulders first
        if long_hair:
            back = (np.abs(u - cx) < 1.25 * hx) & (v > hc - 1.9 * hy) & (v < hc + 0.4 * hy)
            paint(back, _lin(hair) * lit(cx, hc - 0.6 * hy, 1.3 * hx, 1.8 * hy)[..., None] * 0.85, 1)
        # the torso: shoulders rounded off, a little wider at the top, lit round; arms and a crease each side
        dv = np.clip((v - base) / (0.71 * H_), 0, 1)
        halfw = sw * (0.86 + 0.14 * dv)
        cap = top - 0.1 * H_
        sh = np.sqrt(np.clip(1 - ((v - cap) / (0.1 * H_)) ** 2, 0, 1))
        halfw = np.where(v > cap, halfw * (0.42 + 0.58 * sh), halfw)
        hips = base + 0.4 * H_
        if outfit == 'dress':
            # a long skirt flaring to the floor
            halfw = np.where(v < hips, sw * (0.86 + 0.5 * np.clip((hips - v) / (hips - base), 0, 1)), halfw)
            torso = (np.abs(u - cx) < halfw) & (v < top) & (v > base - 0.01)
        elif not full:
            torso = (np.abs(u - cx) < halfw) & (v < top)
        else:
            torso = (np.abs(u - cx) < halfw) & (v < top) & (v > hips - 0.02 * H_)
            # legs, in trousers (dark) or bare from the knee (the beach)
            for sx in (-1, 1):
                lc = cx + sx * 0.42 * sw
                lw = 0.36 * sw * (0.75 + 0.25 * np.clip((v - base) / (hips - base), 0, 1))
                leg = (np.abs(u - lc) < lw) & (v < hips + 0.01 * H_) & (v > base)
                lx = (u - lc) / np.maximum(lw, 1e-6)
                ls = 0.6 + 0.4 * np.sqrt(np.clip(1 - lx ** 2, 0, 1))
                paint(leg, _lin(legs) * ls[..., None], 1)
                if legs == skin:
                    paint(leg & (v > base + 0.2 * H_), _lin(shorts) * ls[..., None], 1)
        xr = (u - cx) / np.maximum(halfw, 1e-6)
        ts = 0.62 + 0.38 * np.sqrt(np.clip(1 - xr ** 2, 0, 1))
        ts = ts * np.where(np.abs(xr) > 0.74, 0.86, 1.0) * np.where(np.abs(np.abs(xr) - 0.74) < 0.035, 0.7, 1.0)
        folds = 0.94 + 0.06 * np.sin((u - cx) / max(sw, 1e-6) * 9 + v * 30 + cx * 7)
        paint(torso, _lin(shirt) * (ts * folds)[..., None], 1)
        for sx in (-1, 1) if full else ():
            paint(ell(cx + sx * 0.86 * sw, hips + 0.005 * H_, 0.14 * sw, 0.038 * H_), _lin(skin) * 0.82, 1)
        vee = np.abs(u - cx) < sw * 0.42 * np.clip((v - (top - 0.32 * H_)) / (0.32 * H_), 0, 1)
        if outfit in ('dealer', 'suit'):
            # a white shirt down the middle, a dark vest or jacket either side of it; a tie or a bow tie
            paint(torso & vee, _lin(0xf0ece4) * ts[..., None], 1)
            tie_y = top - 0.035 * H_
            if outfit == 'dealer':
                paint(ell(cx - 0.035 * H_ * asp, tie_y, 0.035 * H_ * asp, 0.018 * H_) | ell(cx + 0.035 * H_ * asp, tie_y, 0.035 * H_ * asp, 0.018 * H_),
                      0x7a1420, 1)
                paint(ell(cx + sw * 0.55, top - 0.2 * H_, 0.06 * H_ * asp, 0.012 * H_), 0xc8a050, 1)
            else:
                paint((np.abs(u - cx) < 0.018 * H_ * asp) & (v < tie_y) & (v > top - 0.3 * H_), 0x1a1a24, 1)
        elif outfit == 'dress':
            paint(torso & (v > top - 0.06 * H_) & (np.abs(u - cx) > sw * 0.3), _lin(skin) * ts[..., None] * 0.95, 1)
        else:
            paint(torso & (np.abs(u - cx) < sw * 0.22) & (v > top - 0.06 * H_), _lin(skin) * 0.8, 1)
        # neck and ears, then the face, lit round
        paint((np.abs(u - cx) < 0.032 * H_ * asp) & (v > top - 0.05 * H_) & (v < hc - 0.5 * hy), _lin(skin) * 0.72, 1)
        for sx in (-1, 1):
            paint(ell(cx + sx * 0.97 * hx, hc - 0.05 * hy, 0.18 * hx, 0.24 * hy), _lin(skin) * 0.8, 1)
        paint(ell(cx, hc, hx, hy), _lin(skin) * lit(cx, hc, hx, hy)[..., None], 1)
        # the features: soft, as a snapshot catches them
        for sx in (-1, 1):
            paint(ell(cx + sx * 0.36 * hx, hc + 0.1 * hy, 0.2 * hx, 0.075 * hy), _lin(skin) * 0.55, 1, 0.8)
            paint(ell(cx + sx * 0.36 * hx, hc + 0.1 * hy, 0.09 * hx, 0.06 * hy), 0x1e140e, 1, 0.85)
            paint(ell(cx + sx * 0.36 * hx, hc + 0.3 * hy, 0.24 * hx, 0.045 * hy), hair, 1, 0.7)
            paint(ell(cx + sx * 0.5 * hx, hc - 0.28 * hy, 0.2 * hx, 0.12 * hy), _lin(skin) * np.array([1.08, 0.9, 0.88]), 2, 0.35)
        paint(ell(cx + 0.07 * hx, hc - 0.12 * hy, 0.08 * hx, 0.17 * hy), _lin(skin) * 0.7, 1, 0.6)
        mouth = ell(cx, hc - 0.46 * hy, 0.3 * hx, 0.065 * hy) & ((v < hc - 0.45 * hy) if smile else True)
        paint(mouth, 0x7a3a34, 1, 0.8)
        if smile:
            paint(ell(cx, hc - 0.47 * hy, 0.22 * hx, 0.03 * hy), 0xe8e0d0, 1, 0.6)
        # hair: a cap over the crown and sides, a sheen across it
        capm = ell(cx, hc + 0.2 * hy, 1.1 * hx, 0.95 * hy) & ~(ell(cx, hc - 0.15 * hy, 0.9 * hx, 0.86 * hy) & (v < hc + 0.45 * hy))
        hs = lit(cx, hc + 0.2 * hy, 1.1 * hx, 0.95 * hy) * (1 + 0.35 * np.exp(-((v - (hc + 0.62 * hy)) / (0.07 * hy)) ** 2))
        paint(capm, _lin(hair) * hs[..., None], 1)
        if long_hair:
            sides = (np.abs(u - cx) > 0.8 * hx) & (np.abs(u - cx) < 1.22 * hx) & (v > hc - 1.5 * hy) & (v < hc + 0.3 * hy)
            paint(sides, _lin(hair) * 0.8, 1)

    def bokeh(n, y0, y1, col, r, strength=1.0):
        for _ in range(n):
            bx, by = rng.random(), y0 + (y1 - y0) * rng.random()
            rr = r * (0.7 + 0.6 * rng.random())
            paint(ell(bx, by, rr * asp, rr), _lin(col) * (0.6 + 0.6 * rng.random()), 2, 0.55 * strength)

    if kind_ == 'dealer':
        # 1998: Dee at the Riverside, in the vest and bow tie, cards and chips in front of her
        img[:] = _lin(0x2e1c12) * (0.55 + 0.6 * v[..., None])
        bokeh(10, 0.6, 1.0, 0xffcf8a, 0.035)
        paint(ell(0.25, 0.92, 0.18, 0.05), 0xffd89a, 4, 0.5)
        paint(ell(0.78, 0.95, 0.15, 0.04), 0xffd89a, 4, 0.4)
        person(0.5, 0.2, 0.74, 0x161616, skin=0xd8a684, hair=0x6a3a1e, outfit='dealer', long_hair=True)
        paint(ell(0.5, -0.06, 0.95, 0.36), _lin(0x1f5a34) * (0.75 + 0.3 * v[..., None]), 1)
        paint(ell(0.5, -0.06, 0.95, 0.36) & ~ell(0.5, -0.06, 0.9, 0.32), 0x4a2a16, 1)
        for k, cc in enumerate((0xc81e1e, 0x1e4ac8, 0x1a1a1a, 0x2e8a3a, 0xe8e8e8, 0xc81e1e)):
            bx = 0.24 + k * 0.105
            for j in range(3 + (k * 7) % 4):
                paint(ell(bx, 0.07 + j * 0.016, 0.04, 0.012), _lin(cc) * (0.8 + 0.2 * (j % 2)), 1)
                paint(ell(bx, 0.07 + j * 0.016, 0.04, 0.012) & (np.abs(((u - bx) / 0.04 * 3) % 1 - 0.5) < 0.12), 0xf0ece4, 0, 0.7)
        for k in range(2):
            paint((np.abs(u - (0.44 + k * 0.08)) < 0.03) & (np.abs(v - 0.2) < 0.03), 0xf2f0ea, 1)
    elif kind_ == 'beach':
        img[:] = _lin(0x8ab4d8) * (0.75 + 0.35 * v[..., None]) + _lin(0xd8d0c0) * np.clip(0.6 - v, 0, 1)[..., None] * 0.5
        sea = v < 0.47
        paint(sea, _lin(0x2c6688) * (0.7 + 0.5 * (v[..., None] / 0.47)), 0)
        glint = soften(rng.random((h, w)), sp) > 0.62
        paint(sea & glint & (np.abs(np.sin(u * 90 + v * 260)) > 0.9) & (v > 0.32), 0xe8eef0, 1, 0.3)
        paint(v < 0.3 + 0.012 * np.sin(u * 22), 0xe8ecec, 1, 0.7)
        sand = v < 0.27 + 0.01 * np.sin(u * 17)
        paint(sand, _lin(0xd6c094) * (0.8 + 0.25 * soften(rng.random((h, w)), 2 * sp)[..., None]), 1)
        person(0.4, 0.04, 0.64, 0xc84a3a, skin=0xd09a74, hair=0x5a3018, long_hair=True, legs=0xd09a74, shorts=0xe8e0d0, full=True)
        person(0.63, 0.04, 0.7, 0x3a6ac8, skin=0xb07850, hair=0x1a1410, legs=0xb07850, shorts=0x2a3a4a, full=True)
    elif kind_ == 'school':
        img[:] = _lin(0x5a7aa8) * (0.7 + 0.45 * soften(rng.random((h, w)), 10 * sp)[..., None]) * (0.85 + 0.25 * v[..., None])
        person(0.5, -0.22, 1.08, 0x8a2a3a, skin=0xd8a888, hair=0x7a4a24, long_hair=True)
    elif kind_ == 'party':
        img[:] = _lin(0x2a1a14) * (0.6 + 0.6 * v[..., None])
        for k in range(18):
            bx = k / 17
            paint(ell(bx, 0.9 - 0.06 * np.sin(bx * np.pi), 0.014 * asp, 0.014), 0xffd890, 2, 0.9)
        bokeh(8, 0.55, 0.85, 0xff9a5a, 0.03, 0.6)
        for k, cx in enumerate((0.84, 0.16, 0.66, 0.34, 0.5)):
            person(cx, -0.05 + (0.05 if k < 2 else 0.0), 0.62 + 0.12 * (k >= 2) + 0.06 * rng.random(),
                   [0x8a1a2a, 0x1a1a1a, 0xc8a040, 0x2a3a6a, 0x5a2a5a][k],
                   skin=[0xd4a080, 0x8a5a3a, 0xe0b090, 0xc89070, 0xb07850][k], hair=[0x2a1a10, 0x101010, 0x8a5a2a, 0x3a2414, 0x1a1210][k],
                   long_hair=k in (2, 4))
    elif kind_ == 'wedding':
        img[:] = _lin(0xc8b898) * (0.65 + 0.4 * v[..., None])
        paint((np.abs(u - 0.5) < 0.22) & (v > 0.35), _lin(0xf0e8d8) * 0.95, 3, 0.6)
        paint(ell(0.5, 0.82, 0.22, 0.2), _lin(0xfff4e0), 4, 0.5)
        person(0.62, 0.0, 0.9, 0x1a1a1a, skin=0xd0a080, hair=0x2a1a10, outfit='suit', full=True)
        # the veil behind the bride, then the bride
        paint(ell(0.38, 0.62, 0.12, 0.34), _lin(0xf4f0e8), 3, 0.55)
        person(0.38, 0.0, 0.84, 0xf2eee4, skin=0xd8b090, hair=0x3a2010, outfit='dress', long_hair=True, full=True)
        for bx, by, bc in ((0.37, 0.36, 0xc84a5a), (0.4, 0.37, 0xe8d8d0), (0.385, 0.34, 0xd86a7a), (0.36, 0.33, 0xf0e8e0), (0.4, 0.33, 0xb83a4a)):
            paint(ell(bx, by, 0.022, 0.02), bc, 1)
        g = img.mean(axis=2, keepdims=True)
        img = g * np.array([1.06, 0.93, 0.72])
    elif kind_ == 'lighthouse':
        img[:] = _lin(0xf0a868) * np.clip(1.15 - v, 0, 1)[..., None] + _lin(0x46669a) * np.clip(v - 0.15, 0, 1)[..., None]
        paint(ell(0.25, 0.4, 0.12, 0.05), 0xffd8a0, 4, 0.6)
        sea = v < 0.32
        paint(sea, _lin(0x2a4a5e) * (0.7 + 0.6 * (v[..., None] / 0.32)), 0)
        glint = soften(rng.random((h, w)), sp) > 0.6
        paint(sea & glint & (np.abs(np.sin(v * 300)) > 0.8) & (np.abs(u - 0.25) < 0.1 + 0.3 * (0.32 - v)), 0xffd8a0, 1, 0.5)
        paint(ell(0.42, 0.22, 0.42, 0.11) & (v > 0.15), _lin(0x3a342e) * (0.7 + 0.4 * soften(rng.random((h, w)), 3 * sp)[..., None]), 1)
        tower = (np.abs(u - 0.62) < (0.045 - 0.015 * v) * 1.0) & (v > 0.27) & (v < 0.78)
        txr = (u - 0.62) / (0.045 - 0.015 * v)
        tw = 0.6 + 0.45 * np.sqrt(np.clip(1 - txr ** 2, 0, 1)) - 0.12 * txr
        paint(tower, _lin(0xf0ece4) * tw[..., None], 1)
        paint(tower & (np.abs(v - 0.55) < 0.04), _lin(0xb02020) * tw[..., None], 0)
        paint((np.abs(u - 0.62) < 0.05) & (np.abs(v - 0.79) < 0.012), 0x1a1a1a, 1)
        paint(ell(0.62, 0.82, 0.028, 0.03), 0xfff0b0, 1)
        paint(ell(0.62, 0.82, 0.09, 0.06), 0xfff0c0, 4, 0.35)
    else:
        base = {'cover_cards': 0x0e3a24, 'cover_food': 0xd8c8a8, 'cover_tv': 0x2a3a8a, 'cover_home': 0x8aa070}.get(kind_, 0x808080)
        img[:] = _lin(base) * (0.75 + 0.35 * v[..., None])
        if kind_ == 'cover_cards':
            for k in range(5):
                cx = 0.22 + 0.09 * k
                paint((np.abs(u - cx) < 0.07) & (np.abs(v - 0.38 - 0.03 * k) < 0.15), 0xf0ece4, 1)
                paint(ell(cx, 0.42 + 0.03 * k, 0.025, 0.03), 0xb3141c if k % 2 else 0x161616, 1)
            person(0.78, -0.02, 0.72, 0x1a1a1a, skin=0xc89070, outfit='suit', smile=False)
        elif kind_ == 'cover_food':
            paint(ell(0.5, 0.42, 0.38, 0.27), _lin(0xf0ece4) * lit(0.5, 0.42, 0.38, 0.27)[..., None], 1)
            paint(ell(0.5, 0.42, 0.29, 0.2), _lin(0x9a4418) * lit(0.48, 0.45, 0.29, 0.2)[..., None], 2)
            for k in range(6):
                paint(ell(0.4 + 0.04 * k, 0.4 + 0.05 * np.sin(k * 2.0), 0.03, 0.025), 0xc87a2a if k % 2 else 0x5a7a2a, 1)
        elif kind_ == 'cover_tv':
            person(0.33, -0.02, 0.86, 0xc83a3a, skin=0xd8a888, hair=0xc8a050, long_hair=True)
            person(0.68, -0.02, 0.92, 0x1a1a3a, skin=0xc89070, outfit='suit')
        else:
            paint(v < 0.35, _lin(0x7a5e40) * (0.8 + 0.3 * u[..., None]), 2)
            paint((np.abs(u - 0.5) < 0.25) & (v > 0.3) & (v < 0.6), _lin(0xe8dcc0) * (0.85 + 0.2 * (u[..., None] < 0.5)), 1)
            paint(ell(0.5, 0.62, 0.3, 0.12) & (v > 0.6), 0x8a2a1a, 1)
            for k in range(2):
                paint((np.abs(u - (0.4 + 0.2 * k)) < 0.04) & (np.abs(v - 0.46) < 0.05), 0xf8e8a0, 1)
    img = soften(img, 2 * sp)
    # the flash's falloff, a soft vignette, grain; the years: blacks lifted, the blues gone, a warm cast
    vig = 1.0 - 0.38 * (((u - 0.5) / 0.6) ** 2 + ((v - 0.5) / 0.6) ** 2)
    img = img * vig[..., None] * (0.93 + 0.14 * soften(rng.random((h, w)), 6 * sp)[..., None])
    img = np.clip(img + rng.normal(0, 0.014, img.shape), 0, 1)
    img = img * np.array([1.04, 0.98, 0.86]) * 0.93 + 0.022
    return np.clip(img, 0, 1)


# ================================================================== pillows

def pillow(name, w, h, t, n=(26, 26), pull=0.05, dent=None, flat_back=0.0, wrinkle=0.0, seed=0):
    """A knife-edge pillow lying in XY, its faces bulging +Z / -Z to t / 2, the seam at z = 0 (corners pinched, sides
    drawn in). dent = (x, y, radius, depth) on top. Returns (object, seam points) in local coordinates; 'pat' UVs
    run 0..1 over each face."""
    nu, nv = n
    rng = np.random.default_rng(seed)
    us = np.linspace(-1, 1, nu + 1)
    vs = np.linspace(-1, 1, nv + 1)
    bm = bmesh.new()
    top = {}
    bot = {}

    def xy(u, v):
        x = u * w / 2 * (1 - pull * (1 - v * v))
        y = v * h / 2 * (1 - pull * (1 - u * u))
        return x, y

    def thick(u, v):
        f = max((1 - u * u) * (1 - v * v), 0.0) ** 0.42
        return t / 2 * f

    pat = {}
    for i, u in enumerate(us):
        for j, v in enumerate(vs):
            x, y = xy(u, v)
            z = thick(u, v)
            if wrinkle > 0:
                z += wrinkle * math.sin(u * 7.0 + v * 3.0 + seed) * (1 - (1 - abs(u)) ** 2) * (abs(u) > 0.3) * z / max(t / 2, 1e-6)
            edge = i in (0, nu) or j in (0, nv)
            zt = z
            if dent is not None:
                dx, dy, rad, dep = dent
                zt -= dep * math.exp(-((x - dx) ** 2 + (y - dy) ** 2) / (rad * rad)) * (z / max(t / 2, 1e-6))
            top[(i, j)] = bm.verts.new((x, y, zt))
            pat[top[(i, j)]] = ((u + 1) / 2, (v + 1) / 2)
            if edge:
                bot[(i, j)] = top[(i, j)]
            else:
                zb = -z * (1.0 - flat_back)
                bot[(i, j)] = bm.verts.new((x, y, zb))
                pat[bot[(i, j)]] = ((u + 1) / 2, (v + 1) / 2)
    for i in range(nu):
        for j in range(nv):
            bm.faces.new((top[(i, j)], top[(i + 1, j)], top[(i + 1, j + 1)], top[(i, j + 1)]))
            bm.faces.new((bot[(i, j + 1)], bot[(i + 1, j + 1)], bot[(i + 1, j)], bot[(i, j)]))
    uv = bm.loops.layers.uv.new('UVMap')
    pl = bm.loops.layers.uv.new('pat')
    for f in bm.faces:
        for l in f.loops:
            l[pl].uv = pat[l.vert]
            l[uv].uv = pat[l.vert]
    seam = []
    ring = [(i, 0) for i in range(nu)] + [(nu, j) for j in range(nv)] + [(i, nv) for i in range(nu, 0, -1)] + [(0, j) for j in range(nv, 0, -1)]
    for key in ring:
        seam.append(tuple(top[key].co))
    obj = core.mesh_object(name, bm)
    obj.data.uv_layers.active = obj.data.uv_layers['UVMap']
    obj.data.shade_smooth()
    return obj, seam


def cord(name, pts, r=0.004, matrix=None, n=8):
    obj = core.sweep(name, [Vector(p) for p in pts], parts.circle(r, n), closed=True)
    if matrix is not None:
        obj.data.transform(matrix)
    return obj


# ================================================================== SM_DR_Couch

C_W = 2.10           # overall width
C_ARM_IN = 0.86      # the arms' inner faces (x)
C_DECK = 0.31        # the deck under the seat cushions
C_FRONT = -0.455
C_BACK = 0.452
C_LEG = 0.095
C_SEAT_Y = (-0.43, 0.143)
C_CUSH = (2 * C_ARM_IN) / 3


def couch_arm_profile(sign=1):
    """The rolled arm's section (x, z) for the right arm (sign 1) or the left (-1)."""
    cx, cz, R = 0.955, 0.57, 0.095
    x_in, x_out = C_ARM_IN, 1.0
    th0 = -math.degrees(math.acos((x_out - cx) / R))
    pts = [(x_in, C_LEG), (x_out, C_LEG), (x_out, cz + R * math.sin(math.radians(th0)))]
    radii = [0.016, 0.016, 0.014]
    for k in range(1, 29):
        a = math.radians(th0 + (180.0 - th0) * k / 28)
        pts.append((cx + R * math.cos(a), cz + R * math.sin(a)))
        radii.append(0.0)
    poly = round_poly(pts, radii, 5)
    if sign < 0:
        poly = [(-x, z) for x, z in poly]
    return ccw(poly)


def seat_cushion_deform(k):
    """Seat cushion k (0 left, 1 middle, 2 right): puffed, dished where people sit; the middle one broken down."""
    hx, hy, hz = C_CUSH / 2 - 0.002, (C_SEAT_Y[1] - C_SEAT_Y[0]) / 2, 0.075

    def f(P, N):
        x, y, z = P[:, 0], P[:, 1], P[:, 2]
        u, v, w = x / hx, y / hy, z / hz
        top = np.clip((w + 1) / 2, 0, 1)
        fu = np.clip(1 - u * u, 0, 1) ** 0.7
        fv = np.clip(1 - v * v, 0, 1) ** 0.7
        P = P.copy()
        P[:, 2] += np.sign(z) * 0.013 * fu * fv * np.clip(np.abs(w) * 1.5 - 0.3, 0, 1)
        P[:, 0] += np.sign(x) * 0.006 * np.clip(1 - w * w, 0, 1) * fv
        P[:, 1] += np.sign(y) * 0.005 * np.clip(1 - w * w, 0, 1) * fu
        if k == 1:
            dip = 0.068 * np.exp(-((x - 0.02) ** 2 / 0.2 ** 2 + (y + 0.03) ** 2 / 0.22 ** 2))
            droop = 0.022 * np.clip((-v - 0.35) / 0.65, 0, 1) ** 1.5 * fu
            wr = 0.007 * np.sin(x * 46.0 + 1.3 * np.sin(y * 9.0)) * np.exp(-((y + 0.17) ** 2) / 0.012) * fu
            P[:, 2] -= (dip + droop) * (0.3 + 0.7 * top)
            P[:, 2] += wr * top
            P[:, 1] -= 0.012 * top * np.clip(-v, 0, 1)
        else:
            cxs = 0.04 if k == 0 else -0.04
            dip = 0.02 * np.exp(-((x - cxs) ** 2 / 0.16 ** 2 + (y + 0.05) ** 2 / 0.2 ** 2))
            P[:, 2] -= dip * (0.3 + 0.7 * top)
            wr = 0.002 * np.sin(x * 38.0 + k) * np.exp(-((y + 0.2) ** 2) / 0.01) * fu
            P[:, 2] += wr * top
        return P
    return f, (hx, hy, hz)


def back_cushion_deform(k):
    hx, hy, hz = C_CUSH / 2 - 0.002, 0.09, 0.262

    def f(P, N):
        x, y, z = P[:, 0], P[:, 1], P[:, 2]
        u, v, w = x / hx, y / hy, z / hz
        fu = np.clip(1 - u * u, 0, 1) ** 0.6
        fw = np.clip(1 - w * w, 0, 1) ** 0.6
        P = P.copy()
        front = np.clip((-v + 1) / 2, 0, 1)
        P[:, 1] += np.sign(y) * 0.028 * fu * fw * np.clip(np.abs(v) * 1.5 - 0.3, 0, 1)
        # the stuffing settles to the bottom: fuller low, a crease across the top third
        P[:, 1] -= 0.012 * np.clip(w - 0.2, 0, 1) * fu * front
        crease = 0.006 * np.exp(-((w - 0.45) ** 2) / 0.01) * fu
        P[:, 1] += crease * front
        if k == 1:
            P[:, 2] -= 0.02 * np.clip(w + 1, 0, 2) / 2
            P[:, 1] += 0.01 * np.sin(u * 3.0) * fw * front
        return P
    return f, (hx, hy, hz)


def couch_materials():
    def wear(m, sep):
        arms = m.math('MULTIPLY', box_mask(m, sep, (None, None, 0.6), (None, None, None), 0.02),
                      ramp(m, m.math('ABSOLUTE', sep.outputs['X']), 0.88, 0.92))
        seat_front = m.math('MULTIPLY', box_mask(m, sep, (-C_ARM_IN, None, 0.38), (C_ARM_IN, -0.38, 0.5), 0.02), 1.0)
        mid = m.math('MULTIPLY', box_mask(m, sep, (-0.25, -0.3, 0.38), (0.25, 0.1, 0.48), 0.05), 0.7)
        back_top = m.math('MULTIPLY', box_mask(m, sep, (-0.9, 0.12, 0.74), (0.9, 0.35, None), 0.04), 0.35)
        return m.math('MAXIMUM', m.math('MAXIMUM', arms, seat_front), m.math('MAXIMUM', mid, back_top))

    def soiled(m, sep):
        hands = m.math('MULTIPLY', box_mask(m, sep, (None, None, 0.56), (None, -0.2, None), 0.03),
                       ramp(m, m.math('ABSOLUTE', sep.outputs['X']), 0.9, 0.95))
        kick = box_mask(m, sep, (None, None, None), (None, -0.38, 0.22), 0.04)
        return m.math('MAXIMUM', hands, m.math('MULTIPLY', kick, 0.8))

    def dust(m, sep):
        return box_mask(m, sep, (-1.0, 0.33, 0.85), (1.0, None, None), 0.01)

    vel = corduroy('drc_cord', 0x52261a, 0x6c3624, 0x2e140c, 0x86604e, 0x2a160e, wear=wear, soiled=soiled, dust=dust,
                 burn=((0.985, -0.33, 0.651), 0.011))
    throw_c, throw_h, throw_size = granny_throw()
    th = core.Mat('drc_throw')
    uv = uv_coords(th)
    img = th.image(throw_c, vector=uv)
    hi = th.image(throw_h, non_color=True, vector=uv)
    th.set('Base Color', img.outputs['Color'])
    th.set('Roughness', th.math('ADD', 0.86, th.math('MULTIPLY', hi.outputs['Color'], -0.08)))
    th.set('Sheen Weight', 0.6)
    th.set('Sheen Roughness', 0.5)
    th.set('Specular IOR Level', 0.25)
    set_bump(th, hi.outputs['Color'], 0.6, 0.004)

    # The soft things: 0 floral tapestry, 1 mustard velvet, 2 its welt, 3 tassels, 4 ticking pillowcase, 5 army blanket
    fc, fh = floral_tapestry()
    so = core.Mat('drc_soft')
    tc, sep = object_coords(so)
    o = tc.outputs['Object']
    k = attr(so, 'dr_k')
    uvp = uv_coords(so)
    fl = so.image(fc, vector=uvp)
    flh = so.image(fh, non_color=True, vector=uvp)
    crush = ramp(so, noise(so, o, scale=7.0, detail=3.0, distortion=2.0), 0.4, 0.65)
    mustard = so.mix(so.math('MULTIPLY', crush, 0.35), H(0x7e520e), H(0x9c7426))
    tassel = so.mix(ramp(so, noise(so, scaled(so, sep, 3000.0, 3000.0, 40.0), detail=2.0), 0.3, 0.7), H(0x5a1420), H(0x8a2a34))
    # ticking: narrow blue stripes along the pillow's length (pat v), on white gone a little grey
    pv = so.node('ShaderNodeSeparateXYZ')
    so.link(uvp, pv.inputs['Vector'])
    sv = so.math('FRACT', so.math('MULTIPLY', pv.outputs['X'], 14.0))
    stripe = so.math('MAXIMUM', smooth_less(so, so.math('ABSOLUTE', so.math('SUBTRACT', sv, 0.5)), 0.07, 0.015),
                     smooth_less(so, so.math('ABSOLUTE', so.math('SUBTRACT', sv, 0.62)), 0.02, 0.01))
    ticking = so.mix(stripe, H(0xdcd9d0), H(0x34507a))
    ticking = so.mix(so.math('MULTIPLY', ramp(so, noise(so, o, scale=12.0, detail=3.0), 0.5, 0.8), 0.35), ticking, H(0xb7ab92))
    wool_n = noise(so, o, scale=900.0, detail=3.0)
    blanket = so.mix(so.math('MULTIPLY', wool_n, 0.5), H(0x4a4a36), H(0x6a6a50))
    band = so.math('MULTIPLY', ramp(so, sep.outputs['X'], 0.78, 0.785), so.math('SUBTRACT', 1.0, ramp(so, sep.outputs['X'], 0.80, 0.805)))
    blanket = so.mix(band, blanket, H(0x8a7a52))
    col = fl.outputs['Color']
    col = so.mix(is_k(so, k, 1), col, mustard)
    col = so.mix(is_k(so, k, 2), col, so.mix(0.4, mustard, H(0x6e4a10)))
    col = so.mix(is_k(so, k, 3), col, tassel)
    col = so.mix(is_k(so, k, 4), col, ticking)
    col = so.mix(is_k(so, k, 5), col, blanket)
    so.set('Base Color', col)
    rough = mix_float(so, is_k(so, k, 1), 0.8, so.math('SUBTRACT', 0.78, so.math('MULTIPLY', crush, 0.25)))
    rough = mix_float(so, is_k(so, k, 4), rough, 0.75)
    rough = mix_float(so, is_k(so, k, 5), rough, 0.92)
    so.set('Roughness', rough)
    so.set('Sheen Weight', 0.7)
    so.set('Specular IOR Level', 0.3)
    hgt = so.math('MULTIPLY', flh.outputs['Color'], is_k(so, k, 0))
    hgt = so.math('ADD', hgt, so.math('MULTIPLY', wool_n, so.math('MULTIPLY', is_k(so, k, 5), 0.8)))
    hgt = so.math('ADD', hgt, so.math('MULTIPLY', noise(so, o, scale=2500.0, detail=1.0), 0.3))
    set_bump(so, hgt, 0.3, 0.0008)
    wd = wood('drc_wood', 0x5a3420, 0x2a170c, axis='Z', ring=220.0, rough=0.4, wear=0.8, grime=0.4)
    return vel, th, so, wd, throw_size


def build_couch():
    """SM_DR_Couch: 2.10 x 0.91 x 0.86 m. Rolled arms (tops 0.65), the deck at 0.31, three loose seat cushions (top
    about 0.47; the middle one broken down to about 0.42) and three back cushions, welts round every cushion and
    the arm fronts, six bun feet."""
    vel, th, so, wd, throw_size = couch_materials()
    out = []
    proxies = []
    # arms
    for s in (1, -1):
        arm = round_prism('arm', couch_arm_profile(s), C_FRONT, 0.43, r0=0.024, r1=0.0, steps=5)
        out.append(arm)
        inset = 0.024 * (1 - math.sin(math.radians(45)))
        ring = offset_poly(couch_arm_profile(s), inset - 0.002)
        y = C_FRONT + 0.024 * (1 - math.cos(math.radians(45)))
        P = np.array([(x, y, z) for x, z in ring])
        N = np.tile([0.0, -1.0, 0.0], (len(P), 1))
        out.append(welt('arm_welt', P, N * 0.7, 0.0045, 0.0))
    # the back frame: a lofted section (y, z) across X, rounded ends
    bprof = [(0.25, C_LEG), (C_BACK, C_LEG), (C_BACK + 0.006, 0.79), (0.44, 0.862), (0.36, 0.866), (0.30, 0.83), (0.272, 0.60), (0.25, 0.33)]
    bprof = round_poly(bprof, [0.012, 0.012, 0.05, 0.03, 0.04, 0.04, 0.0, 0.0], 5)
    back = round_prism('back', [(-y, z) for y, z in bprof], -C_W / 2 + 0.002, C_W / 2 - 0.002, r0=0.035, r1=0.035, steps=5,
                       matrix=Matrix.Rotation(math.radians(-90), 4, 'Z'))
    out.append(back)
    # deck rail and platform under the cushions
    base = soft_box('base', (C_ARM_IN + 0.01, 0.35, (C_DECK - C_LEG) / 2), 0.03, 0.04,
                    deform=lambda P, N: P + np.c_[np.zeros(len(P)), -0.008 * np.clip(-P[:, 1] / 0.35, 0, 1) * np.clip(1 - (P[:, 2] / 0.108) ** 2, 0, 1), np.zeros(len(P))],
                    matrix=Matrix.Translation((0.0, -0.075, (C_DECK + C_LEG) / 2)))
    out.append(base)
    # seat cushions
    rng = random.Random(4)
    seat_tops = []
    for k in range(3):
        xc = -C_ARM_IN + C_CUSH * (k + 0.5)
        f, half = seat_cushion_deform(k)
        yc = (C_SEAT_Y[0] + C_SEAT_Y[1]) / 2 + (0.006 if k == 1 else rng.uniform(-0.006, 0.004))
        M = mat4((xc + rng.uniform(-0.003, 0.003), yc, C_DECK + half[2] - 0.002), rz=rng.uniform(-0.8, 0.8))
        c = soft_box(f'seat{k}', half, 0.042, 0.026, f, M)
        out.append(c)
        proxies.append(c)
        for sgn in (1, -1):
            P, N = soft_loop(half, 0.042, 2, sgn, f, 0.013)
            out.append(welt('seat_welt', P, N, 0.0042, 0.55, M, n=6))
    # back cushions, leaning back on the frame
    for k in range(3):
        xc = -C_ARM_IN + C_CUSH * (k + 0.5)
        f, half = back_cushion_deform(k)
        tilt = -13.0 if k != 1 else -17.0
        M = mat4((xc, 0.222 if k != 1 else 0.235, C_DECK + half[2] + 0.004 - (0.012 if k == 1 else 0.0)), rz=(0.6 if k == 0 else -0.5), rx=tilt)
        c = soft_box(f'backc{k}', half, 0.06, 0.028, f, M)
        out.append(c)
        proxies.append(c)
        for sgn in (1, -1):
            P, N = soft_loop(half, 0.06, 1, sgn, f, 0.013)
            out.append(welt('back_welt', P, N, 0.0042, 0.55, M, n=6))
    for o in out:
        core.assign(o, vel)
        if o.name.startswith(('arm_welt', 'seat_welt', 'back_welt')):
            kind(o, 1)
            set_pat_array(o, np.zeros((len(o.data.vertices), 2)))
        else:
            kind(o, 0)
            wale_pat(o, 'arm' if o.name.startswith('arm') else 'generic')
    proxies += [out[0], out[2], back, base]
    # bun feet
    feet = []
    for x in (-0.965, 0.0, 0.965):
        for y in (-0.36, 0.37):
            foot = lathe('foot', [(0.0, 0.0), (0.021, 0.0), (0.027, 0.006), (0.034, 0.026), (0.036, 0.046), (0.032, 0.068), (0.026, 0.084),
                                  (0.028, 0.09), (0.03, C_LEG + 0.004), (0.0, C_LEG + 0.004)], 32, Matrix.Translation((x, y, 0.0)))
            core.assign(foot, wd)
            feet.append(foot)
    out += feet

    soft = []
    # Dee's pillows against the back cushions, left of the middle: the floral, the mustard velvet beside it.
    flo, seam = pillow('floral', 0.46, 0.46, 0.17, pull=0.06, wrinkle=0.004, seed=1)
    Mf = mat4((-0.40, 0.045, 0.69), rz=9.0, rx=70.0)
    xf(flo, Mf)
    welt_f = cord('floral_cord', seam, 0.005, Mf)
    tassels = []
    for corner in (0, len(seam) // 4, len(seam) // 2, 3 * len(seam) // 4):
        p = Vector(seam[corner])
        d = p.normalized()
        tas = lathe('tassel', [(0.0, 0.0), (0.006, 0.0), (0.012, -0.012), (0.014, -0.04), (0.0, -0.045)], 12,
                    Matrix.Translation(p + d * 0.004) @ (Vector((0, 0, -1)).rotation_difference(d)).to_matrix().to_4x4())
        xf(tas, Mf)
        tassels.append(tas)
    must, seam2 = pillow('mustard', 0.40, 0.40, 0.15, pull=0.05, wrinkle=0.003, seed=2)
    Mm = mat4((-0.08, 0.0, 0.625), rz=-16.0, rx=60.0)
    xf(must, Mm)
    welt_m = cord('mustard_cord', seam2, 0.0045, Mm)
    for o in (flo, welt_f, *tassels):
        soft_clamp(o, (0, 0, 0.462), (0, 0, 1))
    for o in (must, welt_m):
        soft_clamp(o, (0, 0, 0.44), (0, 0, 1))
    kind(flo, 0)
    kind(welt_f, 0)
    for t_ in tassels:
        kind(t_, 3)
    kind(must, 1)
    kind(welt_m, 2)
    soft += [flo, welt_f, must, welt_m] + tassels
    # The player's bedding at the right end: the bed pillow against the arm, the army blanket folded square on it.
    px_, py_ = 0.63, -0.15
    bed, seam3 = pillow('bedpillow', 0.62, 0.42, 0.17, pull=0.03, flat_back=0.6, seed=3, n=(30, 22), wrinkle=0.004)
    xf(bed, mat4((px_, py_, 0.49), rz=88.0))
    bx, by, bz, brot = 0.61, -0.255, 0.556, -7.0
    P = verts_np(bed)
    ca, sa = math.cos(math.radians(-brot)), math.sin(math.radians(-brot))
    lx = (P[:, 0] - bx) * ca - (P[:, 1] - by) * sa
    ly = (P[:, 0] - bx) * sa + (P[:, 1] - by) * ca
    foot = np.clip((0.24 - np.abs(lx)) / 0.04, 0, 1) * np.clip((0.175 - np.abs(ly)) / 0.04, 0, 1)
    over = np.clip(P[:, 2] - (bz - 0.004), 0, None)
    P[:, 2] -= foot * over
    # pressed under it, the pillow bulges out a little round the blanket's edges
    P[:, 2] += 0.008 * np.clip(1 - foot, 0, 1) * np.exp(-((np.abs(lx) - 0.25) ** 2 + 0.0) / 0.003) * (P[:, 2] > 0.5)
    set_verts(bed, P)
    kind(bed, 4)

    # The blanket folded in six: slightly uneven layers, joined by rounded folds on alternate sides.
    blanket = []
    lt = 0.0118
    brng = random.Random(9)
    Mbk = mat4((bx, by, bz), rz=brot)
    for i in range(6):
        z0 = i * lt
        dx, dy = brng.uniform(-0.004, 0.004), brng.uniform(-0.003, 0.003)

        def layer_def(P, N, i=i):
            P = P.copy()
            P[:, 2] += 0.002 * np.sin(P[:, 0] * 23.0 + i) * np.clip(np.abs(P[:, 1]) / 0.16, 0, 1)
            P[:, 2] -= 0.022 * np.clip((np.abs(P[:, 0]) - 0.08) / 0.14, 0, 1) ** 2
            P[:, 0] += 0.003 * np.sin(P[:, 1] * 31.0 + i * 1.7) * (np.abs(P[:, 0]) > 0.2)
            P[:, 1] += 0.0025 * np.sin(P[:, 0] * 27.0 + i * 2.3) * (np.abs(P[:, 1]) > 0.14)
            return P
        sl = soft_box('blanket_layer', (0.22, 0.155, lt / 2 - 0.0004), 0.0052, 0.022, layer_def,
                      Mbk @ mat4((dx, dy, z0 + lt / 2), rz=brng.uniform(-1.0, 1.0)))
        blanket.append(sl)
        if i < 5:
            side = 1 if i % 2 == 0 else -1
            yf = side * (0.155 + 0.001) + dy
            fold = core.sweep('fold', [Vector((x + dx, yf - side * 0.004, z0 + lt - 0.022 * min(max((abs(x) - 0.08) / 0.14, 0.0), 1.0) ** 2)) for x in np.linspace(-0.216, 0.216, 13)], parts.circle(lt * 0.62, 12))
            xf(fold, Mbk)
            blanket.append(fold)
    for o in blanket:
        kind(o, 5)
    soft += [bed] + blanket
    for o in soft:
        core.assign(o, so)
    out += soft
    proxies += [flo, must, bed] + blanket

    # The throw, tossed over the left arm and the back's corner, simulated.
    th_obj = drape('throw', throw_size, 0.028, mat4((-0.78, -0.12, 1.02), rz=8.0), proxies, frames=80, mass=0.5, bend=0.4, gap=0.004,
                   friction=12.0)
    thicken(th_obj, 0.005)
    core.assign(th_obj, th)
    out.append(th_obj)
    couch = finish(out, 'SM_DR_Couch', [vel, th, so, wd], angle=50.0, weighted=False)
    return [couch]


# ================================================================== shared small parts

def amber_glass(name, hex_color=0x5a2c0c, rough=0.04):
    """Heavy tinted glass, baked opaque: a dark, deep color, glossy, with a little film and scuffs."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    film = ramp(m, noise(m, o, scale=60.0, detail=4.0), 0.5, 0.8)
    edge = m.math('MULTIPLY', convex_edges(m, tc, distance=0.002, samples=12, breakup_scale=200.0, gain=4.0), 0.8)
    col = m.mix(edge, H(hex_color), H(_lighten(hex_color, 0.35)))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', rough, m.math('ADD', m.math('MULTIPLY', film, 0.15), m.math('MULTIPLY', edge, 0.2))))
    m.set('Specular IOR Level', 0.7)
    m.set('Coat Weight', 0.6)
    m.set('Coat Roughness', 0.03)
    return m


def key_outline(length=0.052, bow_r=0.012, blade_w=0.0085, seed=0):
    """A house key's outline (meters, the bow at the origin, the blade along +X), with its hole."""
    rng = random.Random(seed)
    pts = []
    a0 = math.radians(28)
    for k in range(25):
        a = a0 + (2 * math.pi - 2 * a0) * k / 24
        pts.append((bow_r * math.cos(a), bow_r * math.sin(a)))
    yb = -blade_w / 2
    x0 = bow_r * math.cos(a0)
    pts += [(x0 + 0.002, yb + 0.0005), (length - 0.004, yb), (length, yb + 0.0025), (length, 0.0015)]
    # the bitting along the top edge, back toward the bow
    x = length - 0.003
    while x > x0 + 0.006:
        d = rng.uniform(0.0, 0.0028)
        pts.append((x, blade_w / 2 - d))
        x -= 0.0022
        pts.append((x, blade_w / 2 - d * 0.3))
        x -= 0.0016
    pts += [(x0 + 0.004, blade_w / 2 + 0.0012), (x0 + 0.001, blade_w / 2 + 0.0008)]
    hole = [(bow_r * 0.62 + 0.0032 * math.cos(2 * math.pi * k / 16) - bow_r * 0.62, 0.0032 * math.sin(2 * math.pi * k / 16)) for k in range(16)]
    hole = [(x - bow_r * 0.45, y) for x, y in hole]
    return ccw(pts), [hole]


# ================================================================== SM_DR_CoffeeTable

T_TOP = 0.45
T_RINGS = [(0.47, -0.17, 0.036, 'haze'), (0.17, -0.06, 0.033, 'haze'), (-0.13, 0.205, 0.04, 'coffee'), (0.23, 0.17, 0.042, 'coffee'),
           (-0.29, -0.2, 0.038, 'haze'), (0.06, -0.215, 0.041, 'coffee'), (0.205, 0.19, 0.041, 'coffee')]


def table_atlas():
    A = Atlas('drt_prints', 640.0, 470.0, 2048)
    s = A.s
    x, y, w, h = A.region('back', 63.5, 88.9)
    card_back(s, x, y)
    for key, rank, st in (('c1', 13, 'H'), ('c2', 7, 'S'), ('c3', 1, 'D')):
        x, y, w, h = A.region(key, 63.5, 88.9)
        card_face(s, x, y, rank, st)
    # the tuck box: front, back, its sides and ends
    for key in ('box_front', 'box_back'):
        x, y, w, h = A.region(key, 66, 92)
        s.rect(x, y, w, h, 0x8e1a22, rough=0.35)
        s.rect(x + 3, y + 3, w - 6, h - 6, 0xf0e6d6, rough=0.35)
        s.rect(x + 4.5, y + 4.5, w - 9, h - 9, 0x8e1a22, rough=0.35)
        if key == 'box_front':
            s.text("DEALER'S", x + w / 2, y + h - 22, 9, 0xf3e9d8, face='Black', align='CENTER', rough=0.35)
            s.text('CHOICE', x + w / 2, y + h - 33, 9, 0xf3e9d8, face='Black', align='CENTER', rough=0.35)
            for k, st in enumerate('SHDC'):
                suit(s, st, x + w / 2 - 15 + k * 10, y + h / 2 - 2, 6.5, 0xf3e9d8)
            s.text('PLAYING CARDS', x + w / 2, y + 22, 5, 0xf3e9d8, face='Bold', align='CENTER', rough=0.35)
            s.text('STANDARD INDEX · 52 + 2', x + w / 2, y + 13, 3.2, 0xe2c8b8, face='Bold', align='CENTER', rough=0.35)
        else:
            s.circle(x + w / 2, y + h / 2, 14, 0xf0e6d6, rough=0.35)
            s.circle(x + w / 2, y + h / 2, 12.5, 0x8e1a22, rough=0.35)
            suit(s, 'S', x + w / 2, y + h / 2, 12, 0xf0e6d6)
    for key, ww, hh in (('box_side', 19, 92), ('box_end', 66, 19)):
        x, y, w, h = A.region(key, ww, hh)
        s.rect(x, y, w, h, 0x8e1a22, rough=0.35)
        if key == 'box_side':
            s.text("DEALER'S CHOICE", x + w / 2 + 2, y + h / 2, 5.5, 0xf3e9d8, face='Black', align='CENTER', rot=math.pi / 2, rough=0.35)
        else:
            s.text('52', x + w / 2, y + h / 2 - 2.5, 6, 0xf3e9d8, face='Black', align='CENTER', rough=0.35)
    x, y, w, h = A.region('deckside', 90, 18)
    s.rect(x, y, w, h, 0xe9e2d2, rough=0.5)
    for k in range(52):
        s.rect(x, y + k * h / 52, w, 0.09, 0xb9ae9a, rough=0.5)
    s.rect(x, y + h - 0.6, w, 0.6, 0x8e1a22, rough=0.4)
    A.fill('white', 12, 12, 0xeeeae0, 0.45)
    # the chip on Dee's key ring: a red $5 from the Riverside
    x, y, w, h = A.region('chip', 40, 40)
    s.rect(x, y, w, h, 0xa8161e, rough=0.4)
    s.circle(x + 20, y + 20, 19.5, 0xa8161e, rough=0.4)
    for k in range(6):
        a = k * math.pi / 3
        s.poly([(x + 20 + 19.6 * math.cos(a - 0.17), y + 20 + 19.6 * math.sin(a - 0.17)), (x + 20 + 19.6 * math.cos(a + 0.17), y + 20 + 19.6 * math.sin(a + 0.17)),
                (x + 20 + 15.5 * math.cos(a + 0.14), y + 20 + 15.5 * math.sin(a + 0.14)), (x + 20 + 15.5 * math.cos(a - 0.14), y + 20 + 15.5 * math.sin(a - 0.14))], 0xf2ede2, rough=0.4)
    s.ring(x + 20, y + 20, 12.5, 13.3, 0xf2ede2, rough=0.4)
    s.text('5', x + 20, y + 15.5, 11, 0xf2ede2, face='Black', align='CENTER', rough=0.4)
    s.text('RIVERSIDE', x + 20, y + 27.5, 3.2, 0xf2ede2, face='Bold', align='CENTER', rough=0.4)
    # magazines (drawn at half size), the newspaper, the crossword book
    covers = [('mag1', 'THE RIVER', 0x0e3a24, 0xf2c14e, 'cover_cards', ['HOLD\'EM: READING', 'THE QUIET TELLS', 'LATE-NIGHT GAMES', 'NAMED']),
              ('mag2', 'TV WEEK', 0xc81e2a, 0xffffff, 'cover_tv', ['FALL PREVIEW', 'EVERY NEW SHOW', 'RATED']),
              ('mag3', 'GOOD TABLE', 0xd8c8a8, 0x8a1a1a, 'cover_food', ['30-MINUTE', 'SUPPERS', 'POT ROAST']),
              ('mag4', 'HEARTH & HOME', 0x5a6a42, 0xf2ead8, 'cover_home', ['FIX IT', 'YOURSELF'])]
    for key, title, bg, ink, ph, lines in covers:
        x, y, w, h = A.region(key, 105, 137.5)
        s.rect(x, y, w, h, bg, rough=0.25)
        A.paste((x + 4, y + 18, w - 8, h - 46), photo(ph, 210, 190, seed=len(key) + ord(key[-1])), rough=0.22)
        s.text(title, x + w / 2, y + h - 22, 17 if len(title) < 10 else 11.5, ink, face='Black', align='CENTER', rough=0.25)
        for k, ln in enumerate(lines[:2]):
            s.text(ln, x + 6, y + 9 - k * 5.5 + 6, 4.6, ink, face='Black', rough=0.25)
        s.rect(x + w - 18, y + 4, 14, 8, 0xffffff, rough=0.3)
        for b in range(12):
            s.rect(x + w - 17 + b * 1.05, y + 5, 0.45, 6, 0x111111, rough=0.3)
    x, y, w, h = A.region('mag_back', 105, 137.5)
    s.rect(x, y, w, h, 0xe8e2d4, rough=0.3)
    A.paste((x + 6, y + 30, w - 12, h - 40), photo('lighthouse', 160, 200, seed=11), rough=0.25)
    s.text('SEE THE COAST', x + w / 2, y + 16, 7, 0x1a2a4a, face='Black', align='CENTER', rough=0.3)
    x, y, w, h = A.region('pages', 40, 12)
    s.rect(x, y, w, h, 0xe9e4d6, rough=0.6)
    for k in range(24):
        s.rect(x, y + k * h / 24, w, 0.15, 0xc8c0b0, rough=0.6)
    x, y, w, h = A.region('news', 150, 95)
    s.rect(x, y, w, h, 0xe3dccb, rough=0.7)
    s.text('NORTHSIDE HERALD', x + w / 2, y + h - 12, 10.5, 0x161616, face='Black', align='CENTER', rough=0.7)
    s.rect(x + 4, y + h - 15.5, w - 8, 0.5, 0x161616, rough=0.7)
    s.text('TRANSIT FARES UP A QUARTER', x + 5, y + h - 25, 6.2, 0x161616, face='Black', rough=0.7)
    A.paste((x + 5, y + 30, 58, 32), photo('party', 160, 90, seed=21), rough=0.6)
    for col_ in range(3):
        for ln in range(14):
            cx = x + 68 + col_ * 26
            if ln * 4.4 + 8 < h - 32:
                s.rect(cx, y + 8 + ln * 4.4, 23 - (5 if ln % 6 == 5 else 0), 1.6, 0x55524c, rough=0.7)
    for ln in range(5):
        s.rect(x + 5, y + 8 + ln * 4.4, 58, 1.6, 0x55524c, rough=0.7)
    x, y, w, h = A.region('xword', 67.5, 105)
    s.rect(x, y, w, h, 0xf2d23a, rough=0.35)
    s.text('JUMBO', x + w / 2, y + h - 16, 11, 0x1a1a1a, face='Black', align='CENTER', rough=0.35)
    s.text('CROSSWORDS', x + w / 2, y + h - 26, 7.6, 0xb3141c, face='Black', align='CENTER', rough=0.35)
    gx, gy, cs = x + 11, y + 14, 3.8
    for i in range(12):
        for j in range(12):
            black = ((i * 7 + j * 3) % 5 == 0) and not (i in (0, 11) or j in (0, 11))
            s.rect(gx + i * cs, gy + j * cs, cs - 0.35, cs - 0.35, 0x111111 if black else 0xffffff, rough=0.35)
    s.text('VOL. 12  ·  200 PUZZLES', x + w / 2, y + 6, 3.6, 0x1a1a1a, face='Bold', align='CENTER', rough=0.35)
    A.render()
    return A


def table_wood_extra(m, sep, tc, col, rough):
    o = tc.outputs['Object']
    top = m.math('GREATER_THAN', sep.outputs['Z'], T_TOP - 0.0015)
    breakup = noise(m, o, scale=40.0, detail=3.0)
    for i, (cx, cy, rad, kind_) in enumerate(T_RINGS):
        dx = m.math('SUBTRACT', sep.outputs['X'], cx)
        dy = m.math('SUBTRACT', sep.outputs['Y'], cy)
        d = m.math('SQRT', m.math('ADD', m.math('MULTIPLY', dx, dx), m.math('MULTIPLY', dy, dy)))
        wr = m.math('ADD', rad, m.math('MULTIPLY', m.math('SUBTRACT', breakup, 0.5), 0.005))
        dd = m.math('ABSOLUTE', m.math('SUBTRACT', d, wr))
        # the mug sat a little tilted or was dragged: the ring heavier on one side
        a = 1.3 + i * 2.1
        side = m.math('ADD', 0.5, m.math('MULTIPLY', m.math('ADD', m.math('MULTIPLY', dx, math.cos(a)), m.math('MULTIPLY', dy, math.sin(a))), 0.5 / rad))
        wob = noise(m, o, scale=70.0 + i * 13.0, detail=3.0)
        # broken: the ring fades out over part of its round
        partial = ramp(m, noise(m, o, scale=26.0 + i * 4.0, detail=3.0), 0.42, 0.6)
        if kind_ == 'coffee':
            # a dried coffee ring: a thin dark rim (where the drop dried), a faint tan inside it
            width = m.math('ADD', 0.0004, m.math('MULTIPLY', m.math('ADD', m.math('MULTIPLY', side, 0.0011), m.math('MULTIPLY', wob, 0.0006)), 1.0))
            line = ramp(m, m.math('SUBTRACT', width, dd), -0.0002, 0.0003)
            fill = m.math('MULTIPLY', m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', wr, d), -0.0015, 0.0015), ramp(m, wob, 0.3, 0.7)), 0.16)
            stain = m.math('MULTIPLY', m.math('MULTIPLY', m.math('MAXIMUM', m.math('MULTIPLY', line, 0.7), fill), partial), top)
            col = m.mix(m.math('MULTIPLY', stain, 0.68), col, H(0x34200f))
            rough = m.math('SUBTRACT', rough, m.math('MULTIPLY', stain, 0.05))
        else:
            # a water mark clouded into the lacquer: a soft milky band, blotchy, a fainter haze inside
            width = m.math('ADD', 0.0016, m.math('MULTIPLY', m.math('ADD', side, wob), 0.0018))
            line = ramp(m, m.math('SUBTRACT', width, dd), -0.0022, 0.0022)
            fill = m.math('MULTIPLY', ramp(m, m.math('SUBTRACT', wr, d), -0.004, 0.004), 0.22)
            cloud = ramp(m, noise(m, o, scale=160.0, detail=3.0), 0.25, 0.75)
            stain = m.math('MULTIPLY', m.math('MULTIPLY', m.math('MULTIPLY', m.math('MAXIMUM', line, fill), partial), top),
                           m.math('ADD', 0.55, m.math('MULTIPLY', cloud, 0.45)))
            col = m.mix(m.math('MULTIPLY', stain, 0.3), col, H(0xd0c2a8))
            rough = m.math('ADD', rough, m.math('MULTIPLY', stain, 0.3))
    # scratches, mostly along the top
    total = None
    for angle, sx, sy, seed in ((0.08, 7.0, 240.0, 0.0), (-0.7, 5.0, 170.0, 3.1), (1.1, 4.0, 150.0, 7.7)):
        mp = m.node('ShaderNodeMapping')
        mp.inputs['Rotation'].default_value = (0.0, 0.0, angle)
        mp.inputs['Scale'].default_value = (sx, sy, 1.0)
        mp.inputs['Location'].default_value = (seed, seed * 0.7, 0.0)
        m.link(o, mp.inputs['Vector'])
        vor = m.node('ShaderNodeTexVoronoi')
        vor.feature = 'DISTANCE_TO_EDGE'
        m.link(mp.outputs['Vector'], vor.inputs['Vector'])
        lines = smooth_less(m, vor.outputs['Distance'], 0.016, 0.012)
        sparse = ramp(m, noise(m, o, scale=6.0 + seed, detail=2.0), 0.5, 0.68)
        layer = m.math('MULTIPLY', lines, sparse)
        total = layer if total is None else m.math('MAXIMUM', total, layer)
    sc = m.math('MULTIPLY', m.math('MULTIPLY', total, top), 0.75)
    col = m.mix(m.math('MULTIPLY', sc, 0.45), col, H(0xc8a070))
    rough = m.math('ADD', rough, m.math('MULTIPLY', sc, 0.2))
    # the top's front edge rubbed by shins and feet up on it; dust on the shelf
    shelf = m.math('MULTIPLY', box_mask(m, sep, (None, None, 0.115), (None, None, 0.125), 0.002), ramp(m, noise(m, o, scale=90.0, detail=3.0), 0.35, 0.8))
    col = m.mix(m.math('MULTIPLY', shelf, 0.25), col, H(0x9a9080))
    return col, rough


def build_table():
    """SM_DR_CoffeeTable: 1.10 x 0.60 x 0.45 m, honey oak: a bullnosed top on an apron and four turned legs, a lower
    shelf at 0.12. On top, clear of the laptop's place in the middle (LAPTOP_POINT): the deck and its box, three
    loose cards, the amber ashtray holding Dee's keys (a $5 Riverside chip on the ring), a cork coaster. On the shelf:
    magazines, the Northside Herald, the crossword book and a pen."""
    A = table_atlas()
    wd = wood('drt_wood', 0x9a6634, 0x553014, axis='X', ring=150.0, rough=0.3, wear=0.8, grime=0.35, grime_z=(0.0, 0.12), pores=1.0,
              center=(0.03, T_TOP - 0.11), figure=0.8, extra=table_wood_extra)
    pr = print_material('drt_print', A)
    gl = amber_glass('drt_glass')
    mt = street.bare_metal('drt_metal', 0xa8a49a, rough=0.3, grime=0.5)
    misc = core.Mat('drt_misc')
    tc, sep = object_coords(misc)
    k = attr(misc, 'dr_k')
    cork = misc.mix(ramp(misc, noise(misc, tc.outputs['Object'], scale=900.0, detail=2.0), 0.3, 0.7), H(0x4e301a), H(0x7a5432))
    cring = smooth_less(misc, misc.math('ABSOLUTE', misc.math('SUBTRACT', dist3(misc, sep, (0.32, -0.16, T_TOP + 0.004)), 0.031)), 0.0016, 0.001)
    cork = misc.mix(misc.math('MULTIPLY', cring, 0.6), cork, H(0x4a2a14))
    col = misc.mix(is_k(misc, k, 1), cork, H(0x1d3c8a))
    col = misc.mix(is_k(misc, k, 2), col, H(0xe9e6de))
    col = misc.mix(is_k(misc, k, 3), col, H(0xb08d3c))
    ash = misc.mix(ramp(misc, noise(misc, tc.outputs['Object'], scale=300.0, detail=4.0), 0.35, 0.7), H(0x2a2724), H(0x77726a))
    col = misc.mix(is_k(misc, k, 4), col, ash)
    misc.set('Base Color', col)
    misc.set('Roughness', mix_float(misc, misc.math('MAXIMUM', is_k(misc, k, 0), is_k(misc, k, 4)), 0.35, 0.92))
    misc.set('Metallic', mix_float(misc, is_k(misc, k, 3), 0.0, 1.0))
    set_bump(misc, noise(misc, tc.outputs['Object'], scale=900.0, detail=2.0), mix_float(misc, is_k(misc, k, 0), 0.02, 0.4), 0.0004)
    out = []

    def add(o, m):
        core.assign(o, m)
        out.append(o)
        return o

    # the top, bullnosed, its corners rounded
    add(parts.panel('top', core.rounded_rect(-0.55, -0.30, 0.55, 0.30, 0.04, steps=8), T_TOP - 0.028, T_TOP, r=0.011, segments=4), wd)
    # apron
    za, zb = 0.348, T_TOP - 0.028
    add(box(-0.47, 0.47, -0.265, -0.245, za, zb, 0.003), wd)
    add(box(-0.47, 0.47, 0.245, 0.265, za, zb, 0.003), wd)
    add(box(-0.505, -0.485, -0.245, 0.245, za, zb, 0.003), wd)
    add(box(0.485, 0.505, -0.245, 0.245, za, zb, 0.003), wd)
    # turned legs: a square block at the apron, then a vase, a taper, a small foot
    leg = [(0.0, 0.004), (0.014, 0.004), (0.017, 0.01), (0.016, 0.022), (0.019, 0.03), (0.018, 0.04), (0.02, 0.1), (0.023, 0.17), (0.027, 0.22),
           (0.029, 0.25), (0.026, 0.275), (0.02, 0.288), (0.025, 0.298), (0.025, 0.306), (0.019, 0.314), (0.021, 0.33), (0.026, 0.338), (0.026, 0.348),
           (0.0, 0.348)]
    for sx in (-1, 1):
        for sy in (-1, 1):
            cx, cy = sx * 0.495, sy * 0.255
            add(lathe('leg', leg, 32, Matrix.Translation((cx, cy, 0.0))), wd)
            add(box(cx - 0.028, cx + 0.028, cy - 0.028, cy + 0.028, 0.346, zb, 0.004), wd)
            add(parts.disc('glide', (cx, cy, 0.002), (0, 0, 1), 0.012, 0.004, 20), mt)
    # the lower shelf
    add(parts.panel('shelf', core.rounded_rect(-0.47, -0.226, 0.47, 0.226, 0.012, steps=4), 0.1, 0.118, r=0.004), wd)
    for sx in (-1, 1):
        add(box(sx * 0.495 - 0.01, sx * 0.495 + 0.01, -0.24, 0.24, 0.085, 0.1, 0.003), wd)

    # On top. The deck's box, the squared deck beside it, three cards.
    z = T_TOP
    add(print_box('tuck', (0.066, 0.092, 0.019), A, {'+z': 'box_front', '-z': 'box_back', '+x': 'box_side', '-x': 'box_side',
                                                      '+y': 'box_end', '-y': 'box_end'}, r=0.0008, matrix=mat4((-0.445, 0.16, z + 0.0095), rz=17.0)), pr)
    add(print_box('deck', (0.0635, 0.0889, 0.017), A, {'+z': 'back', '-z': 'c3'}, default='deckside', r=0.0012,
                  matrix=mat4((-0.43, 0.035, z + 0.0085), rz=-9.0)), pr)
    card = core.rounded_rect(-0.03175, -0.04445, 0.03175, 0.04445, 0.0032, steps=4)
    for i, (key, cx, cy, rz) in enumerate((('c1', -0.33, -0.16, 24.0), ('c2', -0.305, -0.15, 41.0), ('c3', -0.285, -0.135, 58.0))):
        add(print_panel('card', card, 0.0, 0.0003, A, key, bottom='back', side='white', matrix=mat4((cx, cy, z + 0.0002 + i * 0.00035), rz=rz)), pr)
    # The ashtray: heavy amber glass, three rests; Dee's keys in it.
    ax_, ay_ = 0.40, 0.085
    tray = lathe('ashtray', [(0.0, 0.0), (0.056, 0.0), (0.06, 0.004), (0.065, 0.026), (0.066, 0.031), (0.064, 0.034), (0.052, 0.034), (0.047, 0.031),
                             (0.043, 0.02), (0.036, 0.013), (0.0, 0.012)], 64, Matrix.Translation((ax_, ay_, z)))
    for kk in range(3):
        a = math.radians(30 + kk * 120)
        cut = parts.rod('rest', (ax_ + math.cos(a) * 0.04, ay_ + math.sin(a) * 0.04, z + 0.036), (ax_ + math.cos(a) * 0.075, ay_ + math.sin(a) * 0.075, z + 0.036), 0.0065, 20)
        core.boolean(tray, cut)
    add(tray, gl)
    # what's left of the ash from before she quit, ground into the bottom
    ash = parts.panel('ash', [(0.031 * math.cos(2 * math.pi * t / 40) * (1 + 0.08 * math.sin(t * 1.7)), 0.031 * math.sin(2 * math.pi * t / 40) * (1 + 0.08 * math.sin(t * 2.3)))
                              for t in range(40)], z + 0.0121, z + 0.0124)
    xf(ash, Matrix.Translation((ax_, ay_, 0)))
    add(ash, misc)
    kind(ash, 4)
    fz = z + 0.0125
    ring = lathe('keyring', [(0.0125 + 0.0012 * math.cos(2 * math.pi * t / 8), 0.0012 * math.sin(2 * math.pi * t / 8)) for t in range(9)], 32,
                 Matrix.Translation((ax_ - 0.004, ay_ + 0.002, fz + 0.0035)) @ Matrix.Rotation(math.radians(8), 4, 'X'))
    add(ring, mt)
    for i, (ang, length, seed, tint) in enumerate(((25.0, 0.054, 1, 3), (160.0, 0.05, 2, 0), (275.0, 0.04, 3, 0))):
        outline, holes = key_outline(length, 0.0115 if i < 2 else 0.009, 0.0085 if i < 2 else 0.007, seed)
        kobj = parts.outline_prism('key', outline, holes, -0.001, 0.001)
        a = math.radians(ang)
        rx_ = Matrix.Rotation(math.radians(-14), 4, 'Y')
        M = Matrix.Translation((ax_ - 0.004 + math.cos(a) * 0.012, ay_ + 0.002 + math.sin(a) * 0.012, fz + 0.002 + 0.0012 * i)) @ Matrix.Rotation(a, 4, 'Z') @ rx_
        kobj.data.transform(M)
        add(kobj, mt if tint == 0 else misc)
        if tint:
            kind(kobj, 3)
    chip = core.circle_points(0.0, 0.0, 0.0195, 48)
    add(print_panel('chip', chip, -0.00165, 0.00165, A, 'chip', bottom='chip', side='chip',
                    matrix=mat4((ax_ - 0.004 + math.cos(math.radians(220)) * 0.03, ay_ + 0.002 + math.sin(math.radians(220)) * 0.03, fz + 0.012), rz=30.0, ry=-22.0)), pr)
    # cork coaster with its own ring
    add(parts.disc('coaster', (0.32, -0.16, z + 0.002), (0, 0, 1), 0.045, 0.004, 48), misc)
    kind(out[-1], 0)
    # On the shelf: magazines, the paper, the crossword and a pen.
    zs = 0.118
    rng = random.Random(12)
    for i, key in enumerate(('mag3', 'mag4', 'mag2', 'mag1')):
        th_ = 0.0045 if key != 'mag2' else 0.003
        add(print_box('mag', (0.21, 0.275, th_), A, {'+z': key, '-z': 'mag_back'}, default='pages', r=0.0006,
                      matrix=mat4((-0.24 + rng.uniform(-0.015, 0.015), 0.0 + rng.uniform(-0.012, 0.012), zs + 0.0045 * i + th_ / 2), rz=rng.uniform(-7, 7))), pr)
    add(print_box('paper', (0.30, 0.19, 0.008), A, {'+z': 'news'}, default='pages', r=0.002, matrix=mat4((0.2, -0.01, zs + 0.004), rz=-5.0)), pr)
    add(print_box('xword', (0.135, 0.21, 0.012), A, {'+z': 'xword'}, default='pages', r=0.0015, matrix=mat4((0.24, 0.01, zs + 0.008 + 0.006), rz=14.0)), pr)
    pen = lathe('pen', [(0.0, 0.0), (0.0018, 0.0), (0.0034, 0.01), (0.0037, 0.02), (0.0037, 0.13), (0.0032, 0.142), (0.0, 0.145)], 6,
                mat4((0.215, -0.04, zs + 0.0175 + 0.0037), rz=104.0) @ Matrix.Rotation(math.radians(90), 4, 'Y'))
    add(pen, misc)
    kind(pen, 2)
    cap = lathe('cap', [(0.0, 0.0), (0.004, 0.0), (0.004, 0.045), (0.0, 0.047)], 12,
                mat4((0.215, -0.04, zs + 0.0175 + 0.0037), rz=104.0) @ Matrix.Rotation(math.radians(90), 4, 'Y') @ Matrix.Translation((0, 0, 0.1)))
    add(cap, misc)
    kind(cap, 1)
    for o in out:
        if o.data.attributes.get('dr_k') is None:
            kind(o, 0)
    return [finish(out, 'SM_DR_CoffeeTable', [wd, pr, gl, mt, misc], angle=40.0)]


# ================================================================== SM_DR_Armchair

def sweep_var(name, pts, radius, n=16, closed_ends=True):
    """A tube along pts whose radius varies: radius(t, a) with t 0..1 along the path and a the angle around."""
    obj = core.sweep(name, [Vector(p) for p in pts], parts.circle(1.0, n), caps=closed_ends)
    P = verts_np(obj)
    m = n
    rings = len(pts)
    C = np.array([tuple(p) for p in pts])
    ang = 2 * np.pi * np.arange(m) / m
    for i in range(rings):
        t = i / max(rings - 1, 1)
        sl = slice(i * m, (i + 1) * m)
        d = P[sl] - C[i]
        P[sl] = C[i] + d * np.array([radius(t, a) for a in ang])[:, None]
    set_verts(obj, P)
    obj.data.shade_smooth()
    return obj


def vinyl(name, base, wear_fn=None, oil_fn=None, crack_scale=110.0):
    """Brown vinyl upholstery ('leather' from a furniture store in 1996): a pebbled grain, satin; where it is used
    hardest it crazes, cracks and flakes off its knit backing; hands and hair have darkened and polished it."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    peb = m.node('ShaderNodeTexVoronoi', Scale=700.0)
    m.link(o, peb.inputs['Vector'])
    pebble = ramp(m, peb.outputs['Distance'], 0.05, 0.5)
    grain = noise(m, o, scale=60.0, detail=3.0)
    col = m.mix(m.math('MULTIPLY', grain, 0.35), H(base), H(_lighten(base, 0.12)))
    col = m.mix(m.math('MULTIPLY', m.math('SUBTRACT', 1.0, pebble), 0.25), col, H(_shade(base, 0.7)))
    rough = m.math('ADD', 0.42, m.math('MULTIPLY', pebble, 0.08))
    height = m.math('MULTIPLY', pebble, 0.5)
    if wear_fn is not None:
        w = m.math('MULTIPLY', wear_fn(m, sep), ramp(m, noise(m, o, scale=12.0, detail=4.0), 0.3, 0.62))
        cr = m.node('ShaderNodeTexVoronoi', Scale=crack_scale)
        cr.feature = 'DISTANCE_TO_EDGE'
        mp = m.node('ShaderNodeMapping')
        mp.inputs['Scale'].default_value = (1.0, 1.0, 1.0)
        m.link(o, mp.inputs['Vector'])
        m.link(mp.outputs['Vector'], cr.inputs['Vector'])
        crack = m.math('MULTIPLY', smooth_less(m, cr.outputs['Distance'], 0.035, 0.015), ramp(m, w, 0.15, 0.4))
        peel = m.math('MULTIPLY', ramp(m, w, 0.5, 0.68), ramp(m, noise(m, o, scale=70.0, detail=5.0), 0.6, 0.64))
        knit = noise(m, scaled(m, sep, 2500.0, 2500.0, 2500.0), detail=1.0)
        backing = m.mix(m.math('MULTIPLY', knit, 0.5), H(0x8d7a60), H(0xb8a888))
        col = m.mix(m.math('MULTIPLY', w, 0.35), col, H(_lighten(base, 0.18)))
        col = m.mix(crack, col, H(0x140c07))
        col = m.mix(peel, col, backing)
        rough = mix_float(m, peel, mix_float(m, crack, rough, 0.7), 0.9)
        height = m.math('SUBTRACT', height, m.math('ADD', m.math('MULTIPLY', crack, 1.2), m.math('MULTIPLY', peel, 1.5)))
    if oil_fn is not None:
        oil = m.math('MULTIPLY', oil_fn(m, sep), ramp(m, noise(m, o, scale=8.0, detail=3.0), 0.25, 0.7))
        col = m.mix(m.math('MULTIPLY', oil, 0.55), col, H(_shade(base, 0.55)))
        rough = mix_float(m, oil, rough, 0.26)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Specular IOR Level', 0.45)
    set_bump(m, height, 0.35, 0.0006)
    return m


def knit(name, base=0xc6b28c, fleck=0x7a5a3a):
    """A chunky cable-knit cardigan, sampled from 'pat' in meters: stockinette V's, two cables, ribbing along the
    button band (pat x < 3.5 cm) and the hems (pat y < 5.5 cm); a heathered oatmeal with darker flecks."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    pu = m.node('ShaderNodeSeparateXYZ')
    m.link(uv_coords(m), pu.inputs['Vector'])
    X, Y = pu.outputs['X'], pu.outputs['Y']
    sw, sh = 0.0062, 0.0054
    a = m.math('MULTIPLY', m.math('ABSOLUTE', m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', X, sw)), 0.5)), 2.0)
    b = m.math('FRACT', m.math('DIVIDE', Y, sh))
    vee = m.math('SUBTRACT', 1.0, m.math('MULTIPLY', m.math('ABSOLUTE', m.math('SUBTRACT', a, b)), 2.6), clamp=True)
    stock = m.math('MULTIPLY', vee, m.math('ADD', 0.65, m.math('MULTIPLY', m.math('SUBTRACT', 1.0, a), 0.35)))
    rib = m.math('ADD', 0.5, m.math('MULTIPLY', m.math('COSINE', m.math('MULTIPLY', X, 2 * math.pi / 0.0058)), 0.5))
    rib = m.math('MULTIPLY', rib, m.math('ADD', 0.8, m.math('MULTIPLY', vee, 0.2)))
    ribmask = m.math('MAXIMUM', smooth_less(m, X, 0.035, 0.002), smooth_less(m, Y, 0.055, 0.002))
    height = mix_float(m, ribmask, stock, rib)
    for c0 in (0.13, 0.37):
        w = 0.042
        xl = m.math('DIVIDE', m.math('SUBTRACT', X, c0), w)
        inside = m.math('MULTIPLY', ramp(m, xl, -0.02, 0.02), m.math('SUBTRACT', 1.0, ramp(m, xl, 0.98, 1.02)))
        wave = m.math('MULTIPLY', m.math('SINE', m.math('MULTIPLY', Y, 2 * math.pi / 0.05)), 0.26)
        s1 = m.math('ABSOLUTE', m.math('SUBTRACT', m.math('SUBTRACT', xl, 0.5), wave))
        s2 = m.math('ABSOLUTE', m.math('ADD', m.math('SUBTRACT', xl, 0.5), wave))
        rope = m.math('MAXIMUM', m.math('SUBTRACT', 1.0, m.math('MULTIPLY', s1, 4.0), clamp=True),
                      m.math('SUBTRACT', 1.0, m.math('MULTIPLY', s2, 4.0), clamp=True))
        rope = m.math('ADD', m.math('MULTIPLY', rope, 1.3), m.math('MULTIPLY', vee, 0.15))
        cable = m.math('MULTIPLY', inside, m.math('SUBTRACT', 1.0, ribmask))
        height = mix_float(m, cable, height, rope)
    heather = noise(m, o, scale=400.0, detail=2.0)
    flecks = ramp(m, noise(m, o, scale=900.0, detail=1.0), 0.7, 0.74)
    col = m.mix(m.math('MULTIPLY', heather, 0.4), H(base), H(_lighten(base, 0.15)))
    col = m.mix(m.math('MULTIPLY', flecks, 0.7), col, H(fleck))
    col = m.mix(m.math('MULTIPLY', m.math('SUBTRACT', 1.0, height), 0.55), col, H(_shade(base, 0.55)))
    m.set('Base Color', col)
    m.set('Roughness', 0.94)
    m.set('Sheen Weight', 0.6)
    m.set('Specular IOR Level', 0.25)
    set_bump(m, m.math('ADD', height, m.math('MULTIPLY', heather, 0.15)), 0.9, 0.0035)
    return m


def build_armchair():
    """SM_DR_Armchair: Dee's recliner, 0.90 x 0.93 x 1.03 m, brown vinyl: pillow-top arms (0.63), a cushion dished
    where she sits (0.47), the closed footrest, a three-pillow bustle back. The vinyl is cracked and flaking on the
    arm fronts, the seat's front edge and the headrest; the handle is on the right. Her oatmeal cable-knit cardigan
    hangs over the left arm, a sleeve down its side."""
    def wear(m, sep):
        x, y, z = sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']
        arms = m.math('MULTIPLY', ramp(m, z, 0.5, 0.6), m.math('MULTIPLY', ramp(m, m.math('ABSOLUTE', x), 0.27, 0.3),
                                                              m.math('SUBTRACT', 1.0, ramp(m, y, -0.3, -0.18))))
        seat = box_mask(m, sep, (-0.25, None, 0.42), (0.25, -0.3, 0.5), 0.02)
        head = box_mask(m, sep, (-0.18, None, 0.9), (0.18, None, None), 0.04)
        return m.math('MAXIMUM', arms, m.math('MAXIMUM', m.math('MULTIPLY', seat, 0.8), m.math('MULTIPLY', head, 0.55)))

    def oil(m, sep):
        head = box_mask(m, sep, (-0.14, None, 0.86), (0.14, 0.3, 1.0), 0.04)
        hands = m.math('MULTIPLY', box_mask(m, sep, (None, None, 0.55), (None, -0.25, None), 0.03),
                       ramp(m, m.math('ABSOLUTE', sep.outputs['X']), 0.28, 0.32))
        seat = box_mask(m, sep, (-0.18, -0.25, 0.44), (0.18, 0.1, 0.5), 0.05)
        return m.math('MAXIMUM', head, m.math('MAXIMUM', hands, m.math('MULTIPLY', seat, 0.6)))

    pu = vinyl('dra_vinyl', 0x3b2416, wear, oil)
    kn = knit('dra_knit')
    misc = core.Mat('dra_misc')
    tc, sep = object_coords(misc)
    k = attr(misc, 'dr_k')
    oak = misc.mix(ramp(misc, noise(misc, scaled(misc, sep, 30.0, 30.0, 400.0), detail=3.0), 0.3, 0.7), H(0x6a4224), H(0x4a2c16))
    col = misc.mix(is_k(misc, k, 1), oak, H(0x141414))
    col = misc.mix(is_k(misc, k, 2), col, H(0x5a3a22))
    col = misc.mix(is_k(misc, k, 3), col, H(0x9a9a98))
    misc.set('Base Color', col)
    misc.set('Roughness', mix_float(misc, is_k(misc, k, 1), 0.38, 0.7))
    misc.set('Metallic', mix_float(misc, is_k(misc, k, 3), 0.0, 1.0))
    out = []
    coll = []

    def add(o, m, kk=0, col_=True):
        core.assign(o, m)
        kind(o, kk)
        out.append(o)
        if col_:
            coll.append(o)
        return o

    def puff(amount, axis=2, side_amt=0.006):
        def f(P, N):
            P = P.copy()
            hmax = np.abs(P).max(axis=0)
            u = P / np.maximum(hmax, 1e-6)
            oth = [i for i in range(3) if i != axis]
            fo = np.clip(1 - u[:, oth[0]] ** 2, 0, 1) ** 0.7 * np.clip(1 - u[:, oth[1]] ** 2, 0, 1) ** 0.7
            P[:, axis] += np.sign(P[:, axis]) * amount * fo * np.clip(np.abs(u[:, axis]) * 1.5 - 0.3, 0, 1)
            for i in oth:
                P[:, i] += np.sign(P[:, i]) * side_amt * np.clip(1 - u[:, axis] ** 2, 0, 1)
            return P
        return f

    # the base, the arms and their pillow tops
    add(soft_box('body', (0.43, 0.405, 0.15), 0.03, 0.035, puff(0.0, 1, 0.004), Matrix.Translation((0.0, 0.0, 0.18))), pu)
    for s in (-1, 1):
        add(soft_box('arm', (0.083, 0.41, 0.25), 0.045, 0.03, puff(0.012, 0, 0.004), Matrix.Translation((s * 0.355, -0.008, 0.28))), pu)
        add(soft_box('armtop', (0.098, 0.405, 0.058), 0.054, 0.025, puff(0.016, 2, 0.006), Matrix.Translation((s * 0.36, -0.018, 0.566))), pu)
    # the seat, dished where Dee sits
    def seat_def(P, N):
        P = puff(0.016, 2, 0.006)(P, N)
        top = np.clip((P[:, 2] / 0.075 + 1) / 2, 0, 1)
        P[:, 2] -= 0.038 * np.exp(-(P[:, 0] ** 2 / 0.15 ** 2 + (P[:, 1] - 0.02) ** 2 / 0.17 ** 2)) * (0.3 + 0.7 * top)
        P[:, 2] -= 0.012 * np.clip((-P[:, 1] - 0.2) / 0.09, 0, 1) * top
        return P
    add(soft_box('seat', (0.268, 0.29, 0.075), 0.05, 0.022, seat_def, Matrix.Translation((0.0, -0.1, 0.402))), pu)
    # the footrest, folded shut under the seat's front
    add(soft_box('footrest', (0.262, 0.038, 0.13), 0.03, 0.025, puff(0.012, 1, 0.004), mat4((0.0, -0.405, 0.255), rx=-4.0)), pu)
    # the back: its frame, and the bustle of three pillows (the top one the headrest)
    add(soft_box('backframe', (0.43, 0.11, 0.31), 0.06, 0.035, puff(0.01, 1, 0.006), mat4((0.0, 0.325, 0.63), rx=-9.0)), pu)
    for i, (zc, hz, yc, tilt) in enumerate(((0.575, 0.1, 0.205, -9.0), (0.77, 0.1, 0.235, -11.0), (0.965, 0.105, 0.272, -13.0))):
        def pdef(P, N, i=i):
            P = puff(0.04 if i < 2 else 0.045, 1, 0.012)(P, N)
            P[:, 1] += 0.008 * np.sin(P[:, 0] * 9.0 + i) * np.clip(-P[:, 1] / 0.09, 0, 1)
            return P
        add(soft_box('bustle', (0.27, 0.085, hz), 0.075, 0.022, pdef, mat4((0.0, yc, zc), rx=tilt)), pu)
    # the handle on the right arm's outside, the glides
    add(rbox('lever_plate', (0.441, -0.2, 0.32), (0.006, 0.05, 0.07), 0.002), misc, 3, False)
    lever = rbox('lever', (0, 0, 0), (0.022, 0.034, 0.15), 0.009, 3)
    lever.data.transform(mat4((0.457, -0.21, 0.29), rx=-24.0))
    add(lever, misc, 0, False)
    for sx in (-1, 1):
        for sy in (-1, 1):
            add(parts.disc('glide', (sx * 0.39, sy * 0.37, 0.016), (0, 0, 1), 0.022, 0.03, 16), misc, 1, False)

    # The cardigan: its body folded over the left arm and simulated, a sleeve down the arm's outside, five buttons.
    W_, L_ = 0.64, 0.55
    cg = drape('cardigan', (W_, L_), 0.02, mat4((-0.35, -0.09, 0.86), rz=4.0), coll, frames=80, mass=0.35, bend=0.1, gap=0.004,
               friction=16.0)
    thicken(cg, 0.01)
    # 'pat' in meters for the knit
    me = cg.data
    pl = me.uv_layers['pat']
    uv = np.empty(len(pl.data) * 2, np.float32)
    pl.data.foreach_get('uv', uv)
    uv = uv.reshape(-1, 2) * np.array([W_, L_], np.float32)
    pl.data.foreach_set('uv', uv.ravel())
    add(cg, kn, 0, False)
    # buttons along the button band (sheet x near 0: the outer, hanging edge), on the outside
    bm_ = bmesh.new()
    bm_.from_mesh(me)
    bm_.normal_update()
    lay = bm_.loops.layers.uv['pat']
    lay0 = bm_.loops.layers.uv['UVMap']
    band = {}
    for f in bm_.faces:
        for l in f.loops:
            u_, v_ = l[lay].uv
            if 0.007 < u_ < 0.016 and l[lay0].uv.y < 0.8:
                band.setdefault(round(v_ / 0.08), []).append((l.vert.co.copy(), f.normal.copy()))
    bm_.free()
    for key in sorted(band):
        if key < 1 or key > 5:
            continue
        pts = band[key]
        co = sum((p for p, _ in pts), Vector()) / len(pts)
        nrm = sum((n for _, n in pts), Vector()).normalized()
        # out a little from the edge, onto the band
        btn = parts.disc('button', (0, 0, 0), (0, 0, 1), 0.0095, 0.0035, 20)
        btn.data.transform(Matrix.Translation(co + nrm * 0.0035) @ Vector((0, 0, 1)).rotation_difference(nrm).to_matrix().to_4x4())
        add(btn, misc, 2, False)
    # the sleeve, hanging from under the folded body down the arm's outside
    sp = [(-0.47, -0.17, 0.6), (-0.485, -0.19, 0.5), (-0.487, -0.205, 0.4), (-0.483, -0.215, 0.31), (-0.478, -0.222, 0.235)]
    sp = core.catmull_rom(sp, 8)

    def srad(t, a):
        r = 0.04 - 0.012 * t
        r *= 1.0 + 0.06 * math.sin(t * 22.0 + a * 2.0) * (1 - t)
        if t > 0.85:
            r *= 0.88
        return r * (1.0 - 0.15 * max(0.0, math.cos(a)) ** 2)
    slv = sweep_var('sleeve', sp, srad, 16)
    # 'pat' in meters: around and along, the cuff's rib at the end
    L = sum((Vector(b) - Vector(a)).length for a, b in zip(sp, sp[1:]))
    sl_uv = slv.data.uv_layers['UVMap']
    a_ = np.empty(len(sl_uv.data) * 2, np.float32)
    sl_uv.data.foreach_get('uv', a_)
    a_ = a_.reshape(-1, 2)
    pat = np.stack([0.04 + a_[:, 0] * 0.22, (1.0 - a_[:, 1]) * L], axis=1)
    lay = slv.data.uv_layers.new(name='pat')
    lay.data.foreach_set('uv', pat.astype(np.float32).ravel())
    slv.data.uv_layers.active = slv.data.uv_layers['UVMap']
    add(slv, kn, 0, False)
    # the other sleeve, fallen inside onto the seat and lying across it, flattened
    sp2 = core.catmull_rom([(-0.29, -0.06, 0.6), (-0.245, -0.11, 0.535), (-0.19, -0.165, 0.5), (-0.1, -0.21, 0.488), (-0.02, -0.25, 0.478),
                            (0.04, -0.28, 0.474)], 8)
    slv2 = sweep_var('sleeve2', sp2, lambda t, a: (0.036 - 0.008 * t) * (1.0 + 0.05 * math.sin(t * 19.0 + a * 2.0)), 16)
    P2 = verts_np(slv2)
    C2 = np.array(sp2)
    n2 = 16
    for i in range(len(sp2)):
        t = i / (len(sp2) - 1)
        if t > 0.3:
            sl = slice(i * n2, (i + 1) * n2)
            f = min((t - 0.3) / 0.25, 1.0)
            P2[sl, 2] = C2[i, 2] + (P2[sl, 2] - C2[i, 2]) * (1.0 - 0.55 * f)
    set_verts(slv2, P2)
    L2 = sum((Vector(b) - Vector(a)).length for a, b in zip(sp2, sp2[1:]))
    uv2 = slv2.data.uv_layers['UVMap']
    b_ = np.empty(len(uv2.data) * 2, np.float32)
    uv2.data.foreach_get('uv', b_)
    b_ = b_.reshape(-1, 2)
    lay2 = slv2.data.uv_layers.new(name='pat')
    lay2.data.foreach_set('uv', np.stack([0.04 + b_[:, 0] * 0.22, (1.0 - b_[:, 1]) * L2], axis=1).astype(np.float32).ravel())
    slv2.data.uv_layers.active = slv2.data.uv_layers['UVMap']
    add(slv2, kn, 0, False)
    return [finish(out, 'SM_DR_Armchair', [pu, kn, misc], angle=50.0, weighted=False)]


# ================================================================== SM_DR_TVStand

V_W, V_D, V_H = 0.96, 0.45, 0.56          # the stand
TV_W, TV_H = 0.70, 0.56                   # the CRT's front
TV_Y = -0.205                             # its front face
TV_Z0 = V_H + 0.008                       # its bottom (on feet)
SCR = (0.552, 0.414)                      # the visible screen, 4:3
SCR_C = (0.0, TV_Z0 + 0.07 + 0.235)       # its middle (x, z)
SCR_INSET = 0.016

TAPES = [('XMAS 96', 'hand', 0xf2efe6), ('THE LONG FOLD', 'print', 0x1a1a2a), ('GAME TAPES', 'hand', 0xf2efe6), ('MIDNIGHT RIVER', 'print', 0x7a1a1a),
         ("DEE'S SHOWS", 'hand', 0xf2efe6), ('COUNT THE OUTS', 'print', 0x1a3a5a), ('WEDDING', 'hand', 0xf2efe6), ('HIGH CARD', 'print', 0x2a2a2a),
         ('BLANK', 'hand', 0xeae4d6), ('RIVERSIDE 98', 'hand', 0xf2efe6), ('GOLDEN MILE', 'print', 0x5a4a1a)]
DVDS = [('SHORT STACK', 0x1a1a1a), ('THE QUIET TELL', 0x0e2a4a), ('CITY OF NIGHT', 0x3a0e0e), ('ALL IN', 0x101010), ('DEAD MONEY', 0x2a2a10)]


def tv_atlas():
    A = Atlas('drv_prints', 600.0, 420.0, 2048)
    s = A.s
    A.fill('black', 10, 10, 0x0e0e0f, 0.4)
    A.fill('dark', 10, 10, 0x1d1e20, 0.5)
    A.fill('sleeve', 10, 10, 0xe9e5da, 0.55)
    x, y, w, h = A.region('grille', 120, 40)
    s.rect(x, y, w, h, 0x9c9ea0, rough=0.4, metal=0.6)
    for i in range(int(w / 2.2)):
        for j in range(int(h / 2.2)):
            s.circle(x + 1.1 + i * 2.2, y + 1.1 + j * 2.2, 0.62, 0x101012, n=8, rough=0.6)
    x, y, w, h = A.region('logo', 90, 14)
    s.rect(x, y, w, h, 0xb4b6b8, rough=0.35, metal=0.5)
    s.text('KESTREL', x + w / 2, y + 3.5, 8, 0x2a2b2e, face='Black', align='CENTER', tracking=1.3, rough=0.3)
    x, y, w, h = A.region('tv_label', 80, 50)
    s.rect(x, y, w, h, 0xd8d4c8, rough=0.5)
    s.text('KESTREL  CT-2702', x + 4, y + h - 9, 5.5, 0x1a1a1a, face='Black', rough=0.5)
    s.text('COLOR TELEVISION  120V 60Hz  98W', x + 4, y + h - 16, 3.2, 0x1a1a1a, face='Bold', rough=0.5)
    s.text('MFD. MARCH 2001', x + 4, y + h - 22, 3.2, 0x1a1a1a, face='Bold', rough=0.5)
    s.text('CAUTION: RISK OF ELECTRIC SHOCK', x + 4, y + 10, 3.0, 0xa01818, face='Black', rough=0.5)
    s.text('DO NOT REMOVE BACK', x + 4, y + 5, 3.0, 0x1a1a1a, face='Bold', rough=0.5)
    # the DVD player's front and the VCR's
    x, y, w, h = A.region('dvd_front', 215, 25)
    s.rect(x, y, w, h, 0x1a1b1d, rough=0.3)
    s.rect(x + 10, y + 13, 110, 5.5, 0x0c0c0d, rough=0.2)
    s.rect(x + 10, y + 12.6, 110, 0.4, 0x3a3b3e, rough=0.3)
    s.text('KESTREL', x + 12, y + 4, 4.5, 0xc8c8c8, face='Black', tracking=1.2, rough=0.3)
    s.text('DVD/CD', x + 45, y + 4, 3.2, 0x8a8a8a, face='Bold', rough=0.3)
    s.rect(x + 130, y + 6, 46, 13, 0x05080c, rough=0.1)
    s.text('0:00:00', x + 153, y + 9.5, 5, 0x2a6ad8, face='Bold', align='CENTER', rough=0.1)
    for k in range(5):
        s.circle(x + 186 + k * 5.5, y + 12.5, 1.7, 0x3a3b3e, rough=0.4)
    x, y, w, h = A.region('vcr_front', 190, 45)
    s.rect(x, y, w, h, 0x151516, rough=0.35)
    s.rect(x + 8, y + 17, 105, 18, 0x0c0c0c, rough=0.3)
    s.rect(x + 8.5, y + 17.5, 104, 17, 0x1d1d1e, rough=0.25)
    s.text('HQ  4-HEAD HI-FI STEREO', x + 60, y + 9, 3.6, 0x9a9a9a, face='Bold', align='CENTER', rough=0.3)
    s.text('KESTREL', x + 60, y + 38.5, 4.6, 0xbdbdbd, face='Black', align='CENTER', tracking=1.2, rough=0.3)
    s.rect(x + 124, y + 22, 52, 14, 0x030504, rough=0.1)
    for k, lab in enumerate(('REW', 'PLAY', 'FF', 'STOP', 'EJECT')):
        s.rect(x + 122 + k * 12, y + 8, 9, 5, 0x2a2b2c, rough=0.4)
        s.text(lab, x + 126.5 + k * 12, y + 3.5, 2.2, 0x8a8a8a, face='Bold', align='CENTER', rough=0.4)
    x, y, w, h = A.region('vcr_disp', 52, 14)
    s.rect(x, y, w, h, 0x020302, rough=0.1)
    s.text('12:00', x + w / 2 - 4, y + 2.5, 9.5, 0x46ff7a, face='Bold', align='CENTER', rough=0.1)
    s.text('PM', x + w - 6, y + 3, 3.2, 0x2ac85a, face='Bold', align='CENTER', rough=0.1)
    # the remote's face
    x, y, w, h = A.region('remote_top', 50, 210)
    s.rect(x, y, w, h, 0x1c1c1e, rough=0.4)
    s.circle(x + 10, y + h - 12, 4.5, 0xb01818, rough=0.4)
    for i in range(3):
        for j in range(4):
            bx, by = x + 9 + i * 16, y + h - 40 - j * 13
            s.rect(bx, by, 11, 8, 0x5a5c60, rough=0.5)
            s.text(str(1 + i + j * 3 if j < 3 else ['-', '0', '+'][i]), bx + 5.5, by + 2.2, 4.2, 0xe8e8e8, face='Bold', align='CENTER', rough=0.5)
    s.circle(x + 25, y + 70, 14, 0x3a3c40, rough=0.4)
    s.circle(x + 25, y + 70, 6, 0x5a5c60, rough=0.4)
    for k, c in enumerate((0xc82828, 0x28a028, 0xd8c828, 0x2858c8)):
        s.rect(x + 6 + k * 10.5, y + 40, 8, 5, c, rough=0.4)
    s.text('KESTREL', x + 25, y + 12, 5, 0x9a9a9a, face='Black', align='CENTER', rough=0.4)
    # tapes and discs
    for i, (title, kind_, bg) in enumerate(TAPES):
        x, y, w, h = A.region(f'tape{i}', 25, 188)
        s.rect(x, y, w, h, bg, rough=0.5)
        if kind_ == 'hand':
            s.rect(x + 3, y + 18, w - 6, h - 36, 0xf6f3ea, rough=0.6)
            s.text(title, x + w / 2 + 3.5, y + h / 2, 7.5, [0x1a2a8a, 0x1a1a1a, 0x8a1a1a][i % 3], face='Medium', align='CENTER', rot=math.pi / 2, rough=0.6)
        else:
            s.rect(x, y + h - 30, w, 30, _lighten(bg, 0.25), rough=0.4)
            s.text(title, x + w / 2 + 3.2, y + h / 2 - 15, 7, 0xf0e8d0, face='Black', align='CENTER', rot=math.pi / 2, rough=0.35)
            s.text('VIDEO', x + w / 2 + 2, y + 10, 4, 0xd8c8a0, face='Bold', align='CENTER', rough=0.35)
    for i, (title, bg) in enumerate(DVDS):
        x, y, w, h = A.region(f'dvd{i}', 190, 14)
        s.rect(x, y, w, h, bg, rough=0.2)
        s.text(title, x + 8, y + 4.2, 6.2, 0xeeeeee, face='Black', rough=0.2)
        s.text('DVD', x + w - 14, y + 4.5, 4.6, 0xd8d8d8, face='Black', rough=0.2)
    # the doily on the TV: a crocheted pineapple doily, its holes the dark plastic under it
    A.region('doily', 130, 130)
    A.photo('doily', doily_img(900), rough=0.88)
    A.render()
    return A


def build_tvstand():
    """SM_DR_TVStand (0.96 x 0.45 x 0.56 m stand; the set's top at 1.13): a dark woodgrain laminate stand, chipped
    to the particleboard, a DVD player and a VCR blinking 12:00 in the left bay, home and store tapes and a few DVDs
    in the right. On it a 27-inch KESTREL CRT (screen 0.552 x 0.414, its middle at SCR_C, the glass SCR_INSET behind
    the bezel), a lace doily and rabbit ears; the remote on the stand's right. SM_DR_TVStand_Glow is the screen's face
    (UV 0..1, u left to right, v bottom to top as you face it), 1.5 mm in front of the dark glass."""
    A = tv_atlas()

    def lam_extra(m, sep, tc, col, rough):
        o = tc.outputs['Object']
        chip = m.math('MULTIPLY', ramp(m, convex_edges(m, tc, distance=0.0025, samples=12, breakup_scale=30.0, gain=5.0), 0.5, 0.7),
                      ramp(m, noise(m, o, scale=25.0, detail=3.0), 0.5, 0.62))
        speck = ramp(m, noise(m, o, scale=2500.0, detail=1.0), 0.4, 0.6)
        particle = m.mix(speck, H(0x6a5434), H(0xa88e64))
        col = m.mix(chip, col, particle)
        rough = mix_float(m, chip, rough, 0.88)
        # an even film of dust on the top and the shelves, a little heavier at the back corners
        film = m.math('ADD', 0.6, m.math('MULTIPLY', noise(m, o, scale=8.0, detail=2.0), 0.4))
        dust = m.math('MULTIPLY', up_facing(m, 0.6, 0.9), film)
        dust = m.math('MULTIPLY', dust, m.math('ADD', 0.55, m.math('MULTIPLY', ramp(m, sep.outputs['Y'], -0.05, V_D / 2), 0.45)))
        col = m.mix(m.math('MULTIPLY', dust, 0.22), col, H(0x8a857c))
        rough = mix_float(m, m.math('MULTIPLY', dust, 0.7), rough, 0.75)
        return col, rough
    lam = wood('drv_lam', 0x4a3222, 0x2a1a10, axis='X', ring=90.0, rough=0.42, wear=0.0, grime=0.3, pores=0.2, figure=0.5,
               center=(0.05, V_H - 0.2), extra=lam_extra)
    pl = core.Mat('drv_plastic')
    tc, sep = object_coords(pl)
    o = tc.outputs['Object']
    k = attr(pl, 'dr_k')
    # Moulded plastic: a fine stipple (the body's texture), a silver-painted bezel with a little metal flake. The
    # wear is subtle: rubbed edges on the bezel and a few light scuffs, and an even film of dust on what faces up,
    # thicker toward the back where no one wipes (no blotches).
    g = noise(pl, o, scale=1800.0, detail=1.0)
    stip = noise(pl, o, scale=700.0, detail=2.0)
    scuff = pl.math('MULTIPLY', ramp(pl, noise(pl, scaled(pl, sep, 40.0, 400.0, 400.0), detail=2.0), 0.72, 0.8),
                    ramp(pl, noise(pl, o, scale=8.0, detail=2.0), 0.55, 0.7))
    rub = ramp(pl, convex_edges(pl, tc, distance=0.002, samples=12, breakup_scale=60.0, gain=4.0), 0.35, 0.7)
    silver = pl.mix(pl.math('MULTIPLY', g, 0.25), H(0x80827f), H(0x8c8e8c))
    silver = pl.mix(pl.math('MULTIPLY', rub, 0.5), silver, H(0x6e706e))
    col = pl.mix(is_k(pl, k, 1), silver, pl.mix(pl.math('MULTIPLY', stip, 0.6), H(0x232426), H(0x2c2d30)))
    col = pl.mix(is_k(pl, k, 2), col, H(0x0f0f10))
    col = pl.mix(is_k(pl, k, 3), col, H(0x55575b))
    col = pl.mix(pl.math('MULTIPLY', scuff, 0.18), col, H(0x9a9c9a))
    up = up_facing(pl, 0.5, 0.85)
    film = pl.math('ADD', 0.62, pl.math('MULTIPLY', noise(pl, o, scale=9.0, detail=2.0), 0.38))
    back = ramp(pl, sep.outputs['Y'], TV_Y + 0.05, TV_Y + 0.4)
    dust = pl.math('MULTIPLY', pl.math('MULTIPLY', up, film), pl.math('ADD', 0.45, pl.math('MULTIPLY', back, 0.55)))
    col = pl.mix(pl.math('MULTIPLY', dust, 0.24), col, H(0x8a857a))
    pl.set('Base Color', col)
    rough = pl.math('ADD', mix_float(pl, is_k(pl, k, 0), 0.52, 0.36), pl.math('MULTIPLY', g, 0.06))
    rough = mix_float(pl, pl.math('MULTIPLY', rub, 0.6), rough, 0.28)
    rough = mix_float(pl, pl.math('MULTIPLY', dust, 0.6), rough, 0.7)
    pl.set('Roughness', rough)
    pl.set('Metallic', mix_float(pl, is_k(pl, k, 0), 0.0, 0.25))
    set_bump(pl, pl.math('ADD', pl.math('MULTIPLY', g, 0.4), pl.math('MULTIPLY', stip, is_k(pl, k, 1))), 0.05, 0.0001)
    pr = print_material('drv_print', A)
    sc_ = core.Mat('drv_screen')
    tc2, sep2 = object_coords(sc_)
    k2 = attr(sc_, 'dr_k')
    film = ramp(sc_, noise(sc_, tc2.outputs['Object'], scale=14.0, detail=4.0), 0.45, 0.8)
    img, _a, _m, _r = image_surface(sc_, A.paths, uv_coords(sc_), extension='EXTEND')
    glass_col = sc_.mix(film, H(0x1a201d), H(0x2a2e2c))
    sc_.set('Base Color', sc_.mix(is_k(sc_, k2, 1), glass_col, img))
    sc_.set('Emission Color', sc_.mix(is_k(sc_, k2, 1), (0.0, 0.0, 0.0, 1.0), img))
    sc_.set('Emission Strength', 3.0)
    sc_.set('Roughness', sc_.math('ADD', 0.04, sc_.math('MULTIPLY', film, 0.12)))
    sc_.set('Specular IOR Level', 0.65)
    mt = street.bare_metal('drv_metal', 0xc8c8c4, rough=0.22, grime=0.4)
    out = []

    def add(obj, m, kk=0):
        core.assign(obj, m)
        kind(obj, kk)
        out.append(obj)
        return obj

    # The stand.
    hw, hd = V_W / 2, V_D / 2
    add(box(-hw, hw, -hd, hd, V_H - 0.025, V_H, 0.003), lam)
    for s in (-1, 1):
        add(box(s * hw - (0.02 if s > 0 else 0), s * hw + (0.0 if s > 0 else 0.02), -hd + 0.008, hd, 0.04, V_H - 0.025, 0.002), lam)
    add(box(-hw + 0.02, hw - 0.02, -hd + 0.008, hd, 0.04, 0.058, 0.002), lam)
    add(box(-hw + 0.03, hw - 0.03, -hd + 0.035, hd - 0.004, 0.0, 0.04, 0.002), lam)
    add(box(0.11, 0.13, -hd + 0.012, hd, 0.058, V_H - 0.025, 0.002), lam)
    shelf = add(box(-hw + 0.02, 0.11, -hd + 0.015, hd - 0.004, 0.29, 0.308, 0.002), lam)
    P = verts_np(shelf)
    P[:, 2] -= 0.004 * np.clip(1 - ((P[:, 0] + 0.175) / 0.285) ** 2, 0, 1)
    set_verts(shelf, P)
    add(box(0.13, hw - 0.02, -hd + 0.015, hd - 0.004, 0.29, 0.308, 0.002), lam)
    add(box(-hw + 0.004, hw - 0.004, hd - 0.004, hd, 0.04, V_H - 0.025, 0.0), lam)
    # The DVD player and the VCR in the left bay.
    add(print_box('dvd', (0.43, 0.24, 0.05), A, {'-y': 'dvd_front'}, default='dark', r=0.002, matrix=mat4((-0.17, -0.06, 0.308 + 0.027))), pr)
    add(print_box('vcr', (0.38, 0.26, 0.09), A, {'-y': 'vcr_front'}, default='black', r=0.003, matrix=mat4((-0.17, -0.055, 0.058 + 0.047))), pr)
    for fx in (-0.34, 0.0):
        for fy in (-0.16, 0.06):
            add(parts.disc('foot', (fx, fy, 0.308 + 0.001), (0, 0, 1), 0.01, 0.004, 12), pl, 2)
            add(parts.disc('foot', (fx + 0.02, fy, 0.058 + 0.001), (0, 0, 1), 0.01, 0.004, 12), pl, 2)
    # the VCR's clock, lit (52 x 14 mm, at the front's right)
    dw, dh = 52 / 190 * 0.38, 14 / 45 * 0.09
    disp = parts.rbox('vcr_disp', (0, 0, 0), (dw, 0.0008, dh), 0.0)
    pat_faces(disp, box_pat(A, (dw, 0.0008, dh), {'-y': 'vcr_disp'}, default='black'))
    disp.data.transform(Matrix.Translation((-0.17 - 0.19 + 0.38 * (124 + 26) / 190, -0.055 - 0.13 - 0.0004, 0.105 - 0.045 + 0.09 * (22 + 7) / 45)))
    add(disp, sc_, 1)
    # Tapes and DVDs in the right bay.
    rng = random.Random(8)
    x = 0.135
    for i, (title, kind_, bg) in enumerate(TAPES):
        tw = 0.0255
        lean = 0.0 if i < len(TAPES) - 1 else -14.0
        if x + tw > hw - 0.025:
            break
        add(print_box('tape', (tw, 0.105, 0.188), A, {'-y': f'tape{i}'}, default='sleeve' if kind_ == 'hand' else 'black', r=0.0008,
                      matrix=mat4((x + tw / 2 + (0.02 if lean else 0.0), -hd + 0.02 + 0.0525 + rng.uniform(0.0, 0.012), 0.058 + 0.094 + (0.0 if not lean else -0.01)), ry=lean)), pr)
        x += tw + 0.0008
    z = 0.308
    for i, (title, bg) in enumerate(DVDS):
        add(print_box('dvd_case', (0.19, 0.135, 0.014), A, {'-y': f'dvd{i}'}, default='black', r=0.001,
                      matrix=mat4((0.295 + rng.uniform(-0.006, 0.006), -hd + 0.02 + 0.0675, z + 0.007), rz=90.0 + rng.uniform(-3, 3)) @ Matrix.Rotation(math.radians(-90), 4, 'Z')), pr)
        z += 0.0142

    # The CRT.
    tz = TV_Z0
    zc = tz + TV_H / 2
    bez_outline = core.rounded_rect(-TV_W / 2, -TV_H / 2, TV_W / 2, TV_H / 2, 0.022, steps=6)
    sx0, sx1 = SCR_C[0] - SCR[0] / 2 - 0.008, SCR_C[0] + SCR[0] / 2 + 0.008
    sz0, sz1 = SCR_C[1] - SCR[1] / 2 - 0.008 - zc, SCR_C[1] + SCR[1] / 2 + 0.008 - zc
    hole = list(reversed(core.rounded_rect(sx0, sz0, sx1, sz1, 0.03, steps=6)))
    bez = parts.outline_prism('bezel', bez_outline, [hole], 0.0, 0.055)
    # stand it up: its XY to XZ, the front (z = 0) toward -Y
    bez.data.transform(Matrix.Translation((0, TV_Y + 0.055, zc)) @ Matrix.Rotation(math.radians(90), 4, 'X'))
    add(bez, pl, 0)
    # the glass, a little domed, set back in the opening
    def screen_grid(name, ydepth, bulge):
        bm = bmesh.new()
        nx, nz = 32, 24
        vs = []
        for j in range(nz + 1):
            row = []
            for i in range(nx + 1):
                u, v = i / nx, j / nz
                x_ = SCR_C[0] + (u - 0.5) * (SCR[0] + 0.018)
                z_ = SCR_C[1] + (v - 0.5) * (SCR[1] + 0.018)
                d = bulge * (1 - (2 * u - 1) ** 2) * (1 - (2 * v - 1) ** 2)
                row.append(bm.verts.new((x_, ydepth - d, z_)))
            vs.append(row)
        uvl = bm.loops.layers.uv.new('UVMap')
        for j in range(nz):
            for i in range(nx):
                f = bm.faces.new((vs[j][i], vs[j][i + 1], vs[j + 1][i + 1], vs[j + 1][i]))
                for l, (a, b) in zip(f.loops, ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))):
                    l[uvl].uv = ((a / nx - 0.5) * (SCR[0] + 0.018) / SCR[0] + 0.5, (b / nz - 0.5) * (SCR[1] + 0.018) / SCR[1] + 0.5)
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        obj = core.mesh_object(name, bm)
        core.orient_normals(obj, (0, -1, 0))
        obj.data.shade_smooth()
        return obj
    add(screen_grid('crt', TV_Y + SCR_INSET, 0.006), sc_, 0)
    glow_face = screen_grid('crt_glow', TV_Y + SCR_INSET - 0.0015, 0.006)
    # the body: a tapering bell from the bezel back
    rings = []
    for yy, w_, h_, r_ in ((TV_Y + 0.05, TV_W - 0.01, TV_H - 0.012, 0.02), (TV_Y + 0.14, TV_W - 0.03, TV_H - 0.03, 0.03),
                           (TV_Y + 0.3, 0.56, 0.46, 0.05), (TV_Y + 0.43, 0.42, 0.36, 0.05), (TV_Y + 0.47, 0.38, 0.32, 0.04)):
        pts = core.rounded_rect(-w_ / 2, -h_ / 2, w_ / 2, h_ / 2, r_, steps=5)
        rings.append([(px_, yy, zc + 0.01 * (yy - TV_Y) + pz) for px_, pz in pts])
    body = core.loft('tv_body', rings)
    add(body, pl, 1)
    # grilles, the badge, buttons, the standby light
    bz = SCR_C[1] - SCR[1] / 2 - 0.008 - 0.032
    for gx in (-0.24, 0.24):
        g = parts.rbox('grille', (0, 0, 0), (0.12, 0.0008, 0.034), 0.0)
        pat_faces(g, box_pat(A, (0.12, 0.0008, 0.034), {'-y': 'grille'}, default='dark'))
        g.data.transform(Matrix.Translation((gx, TV_Y - 0.0004, bz)))
        add(g, pr)
    lg = parts.rbox('logo', (0, 0, 0), (0.09, 0.0008, 0.014), 0.0)
    pat_faces(lg, box_pat(A, (0.09, 0.0008, 0.014), {'-y': 'logo'}, default='dark'))
    lg.data.transform(Matrix.Translation((0.0, TV_Y - 0.0004, bz + 0.004)))
    add(lg, pr)
    for i in range(5):
        add(rbox('tvbtn', (0.1 + i * 0.016, TV_Y - 0.002, bz - 0.012), (0.011, 0.005, 0.006), 0.0015), pl, 3)
    add(parts.disc('power', (-0.11, TV_Y - 0.002, bz - 0.011), (0, 1, 0), 0.007, 0.006, 20), pl, 3)
    led = parts.disc('led', (-0.088, TV_Y - 0.0012, bz - 0.011), (0, 1, 0), 0.0018, 0.003, 10)
    add(led, sc_, 1)
    pat_faces(led, lambda P, N: A.uv('vcr_disp', np.full(len(P), 0.45), np.full(len(P), 0.55)))
    lab = parts.rbox('tv_label', (0, 0, 0), (0.08, 0.0006, 0.05), 0.0)
    pat_faces(lab, box_pat(A, (0.08, 0.0006, 0.05), {'+y': 'tv_label'}, default='dark'))
    lab.data.transform(Matrix.Translation((0.08, TV_Y + 0.47 + 0.0004, zc + 0.01 * 0.47 + 0.02)))
    add(lab, pr)
    for fx in (-0.3, 0.3):
        for fy in (TV_Y + 0.03, TV_Y + 0.4):
            add(parts.disc('tvfoot', (fx * (0.95 if fy > 0 else 1.0), fy, V_H + 0.004), (0, 0, 1), 0.016, 0.008, 16), pl, 2)
    # On top: the doily and the rabbit ears, on the set's top behind the bezel.
    def set_top(yr):
        # the bezel's top (to 5.5 cm back) stands a little over the bell's
        bell = np.interp(yr, [0.05, 0.14, 0.3, 0.43], [TV_H / 2 - 0.006 + 0.0005, (TV_H - 0.03) / 2 + 0.0014, 0.23 + 0.003, 0.18 + 0.0043])
        return np.where(yr < 0.056, TV_H / 2, bell)
    # a disc with rings inside it (so it follows the set's curved top), its rim scalloped, on the dark body
    doily = doily_disc('doily', A, 'doily', 0.105, 0.0, 0.0012)
    doily.data.transform(mat4((-0.1, TV_Y + 0.172, 0.0), rz=12.0))
    P = verts_np(doily)
    yr = P[:, 1] - TV_Y
    zt = set_top(yr)
    # smooth the step down off the bezel
    zt = np.where((yr > 0.05) & (yr < 0.075), TV_H / 2 + (yr - 0.05) / 0.025 * (set_top(np.array([0.075]))[0] - TV_H / 2), zt)
    P[:, 2] += zc + zt + 0.0007
    set_verts(doily, P)
    add(doily, pr)
    base_c = Vector((0.13, TV_Y + 0.115, zc + float(set_top(np.array([0.115]))[0])))
    add(lathe('ears_base', [(0.0, 0.0), (0.06, 0.0), (0.062, 0.006), (0.055, 0.02), (0.035, 0.034), (0.012, 0.04), (0.0, 0.041)], 40,
              Matrix.Translation(base_c)), pl, 1)
    add(parts.disc('ears_knob', base_c + Vector((0.0, -0.05, 0.016)), (0, 1, 0), 0.012, 0.012, 20), pl, 3)
    for d, L, ext in (((-0.48, 0.12, 0.87), 0.5, 3), ((0.55, 0.28, 0.79), 0.4, 2)):
        dv = Vector(d).normalized()
        p0 = base_c + Vector((0, 0, 0.036)) + dv * 0.012
        radii = [0.0036, 0.0029, 0.0022]
        seg = L / 3
        for s_ in range(3):
            l_ = seg if s_ < ext else seg * 0.12
            a, b = p0, p0 + dv * (l_ + (0.01 if s_ < 2 else 0.0))
            add(rod(a, b, radii[s_], 10, 'ear'), mt)
            p0 = p0 + dv * l_
        add(parts.disc('ear_tip', p0, tuple(dv), 0.0042, 0.006, 12), mt)
    # the cables behind: power and coax, down behind the stand to the floor
    for pts_, r_ in (([(0.14, TV_Y + 0.47, zc - 0.12), (0.16, TV_Y + 0.5, zc - 0.2), (0.17, hd + 0.03, V_H - 0.05), (0.15, hd + 0.025, 0.2), (0.2, hd + 0.03, 0.006),
                       (0.32, hd + 0.08, 0.004)], 0.0035),
                     ([(-0.08, TV_Y + 0.47, zc - 0.09), (-0.1, TV_Y + 0.52, zc - 0.15), (-0.12, hd + 0.04, V_H - 0.02), (-0.15, hd + 0.03, 0.25), (-0.1, hd + 0.04, 0.006),
                       (-0.3, hd + 0.1, 0.004)], 0.003)):
        add(tube(pts_, r_, 10, 10, 'cable'), pl, 2)
    # the remote, on the stand's right
    add(print_box('remote', (0.05, 0.21, 0.02), A, {'+z': 'remote_top'}, default='dark', r=0.006, seg=3,
                  matrix=mat4((0.405, -0.09, V_H + 0.0105), rz=-8.0)), pr)
    stand = finish(out, 'SM_DR_TVStand', [lam, pl, pr, sc_, mt], angle=40.0)
    g = glow([glow_face], 'SM_DR_TVStand_Glow')
    return [stand, g]


# ================================================================== SM_DR_Rug

R_W, R_H, R_T = 2.4, 1.7, 0.008
R_CURL = dict(corner=(1.2, -0.85), fold=0.16, radius=0.05, theta=1.0)


def _tri(x):
    """A triangle wave: 0 at whole numbers, 1 halfway between."""
    return 1.0 - np.abs(2.0 * (np.asarray(x) % 1.0) - 1.0)


def rug_pattern(px=2048):
    """A machine-made Heriz-style rug (numpy, rows top-down, +X right, +Y up): a madder field under a stepped vine
    lattice with flowers at the crossings and stars in the cells; a hooked navy medallion with pendants; hooked corner
    spandrels; a navy main border of stepped rosettes on a zigzag vine between a sawtooth guard and a diamond-chain
    guard; the short ends finished in an ivory flatweave. Then its years: edges that wander as knots do, the pile's
    fibre, abrash bands, the +X end faded by the sun, a traffic path in from the -X end across the front worn down
    to the foundation in places, a coffee stain, old spots, grime round the edges. Returns (color path, surface
    path: R pile height, G wear)."""
    W, Hh = px, int(round(px * R_H / R_W))
    xs = (np.arange(W) + 0.5) / W * R_W - R_W / 2
    ys = (np.arange(Hh) + 0.5) / Hh * R_H - R_H / 2
    X0, Y0 = np.meshgrid(xs, ys[::-1])
    # the knots' irregularity: every edge wanders a millimeter or so
    X = X0 + (_vn2(X0, Y0, 0.005, seed=41, octaves=2) - 0.5) * 0.0028
    Y = Y0 + (_vn2(X0 + 7.3, Y0 - 2.1, 0.005, seed=42, octaves=2) - 0.5) * 0.0028
    c = {k: _lin(v) for k, v in dict(red=0x76271e, redd=0x541a15, rust=0x8e4229, navy=0x1d2436, ivory=0xc2b28c, gold=0xa07e3a,
                                     blue=0x56728a, brown=0x24170f).items()}
    col = np.zeros((Hh, W, 3)) + c['red']

    def put(mask, name):
        nonlocal col
        m = np.clip(np.asarray(mask, float), 0, 1)
        col = col * (1 - m[..., None]) + c[name] * m[..., None]

    hw, hh = R_W / 2, R_H / 2
    ax_, ay_ = np.abs(X), np.abs(Y)
    edge = np.minimum(hw - ax_, hh - ay_)
    field = edge > 0.23

    # ---- the field: a stepped diamond lattice of darker vines (6 mm steps)
    q = 0.006
    Xq, Yq = (np.floor(X / q) + 0.5) * q, (np.floor(Y / q) + 0.5) * q
    cu, cv = 0.21, 0.15
    u = Xq / cu + Yq / cv
    v = Xq / cu - Yq / cv
    put(field & ((np.abs(u - np.round(u)) < 0.055) | (np.abs(v - np.round(v)) < 0.055)), 'redd')
    # flowers where the vines cross, gold and blue in turn
    iu, iv = np.round(X / cu + Y / cv), np.round(X / cu - Y / cv)
    dx, dy = np.abs(X - (iu + iv) / 2 * cu), np.abs(Y - (iu - iv) / 2 * cv)
    oc = np.maximum(np.maximum(dx, dy), (dx + dy) * 0.72)
    alt = ((iu + iv) % 4) == 0
    put(field & (oc < 0.032), 'ivory')
    put(field & (oc < 0.026) & alt, 'gold')
    put(field & (oc < 0.026) & ~alt, 'blue')
    put(field & (np.minimum(np.maximum(dx * 2.6, dy), np.maximum(dx, dy * 2.6)) < 0.024), 'redd')
    put(field & (oc < 0.008), 'ivory')
    # gold buds on the vines halfway between crossings
    fu, fv = np.floor(X / cu + Y / cv) + 0.5, np.floor(X / cu - Y / cv) + 0.5
    for a_, b_ in ((iu, fv), (fu, iv)):
        bx, by = np.abs(X - (a_ + b_) / 2 * cu), np.abs(Y - (a_ - b_) / 2 * cv)
        put(field & (bx / 0.011 + by / 0.011 < 1), 'gold')
    # four-point stars in the cells
    sx_, sy_ = np.abs(X - (fu + fv) / 2 * cu), np.abs(Y - (fu - fv) / 2 * cv)
    put(field & ((sx_ / 0.024 + sy_ / 0.007 < 1) | (sx_ / 0.007 + sy_ / 0.018 < 1)), 'navy')
    put(field & (sx_ / 0.006 + sy_ / 0.006 < 1), 'ivory')

    # ---- the medallion: hooked lozenges, navy then rust, a stepped star at the heart
    md = ax_ / 0.53 + ay_ / 0.37
    s_ = ax_ / 0.53 - ay_ / 0.37
    mh = md - 0.05 * ((s_ * 7 + 0.25) % 1 < 0.5)
    put(mh < 1.045, 'ivory')
    put(mh < 1.0, 'navy')
    gi, gj = np.round(X / 0.065), np.round(Y / 0.065)
    gx, gy = np.abs(X / 0.065 - gi), np.abs(Y / 0.065 - gj)
    cross = ((gx < 0.1) & (gy < 0.34)) | ((gy < 0.1) & (gx < 0.34))
    nb = (mh < 0.95) & (md > 0.84)
    put(nb & cross & ((gi + gj) % 2 == 0), 'gold')
    put(nb & cross & ((gi + gj) % 2 == 1), 'ivory')
    m2 = md - 0.035 * ((s_ * 5 + 0.25) % 1 < 0.5)
    put(m2 < 0.78, 'ivory')
    put(m2 < 0.75, 'rust')
    put(md < 0.64, 'ivory')
    put(md < 0.62, 'red')
    sq1 = np.maximum(ax_, ay_ * 1.15)
    sq2 = (ax_ + ay_ * 1.15) / 1.4142
    put(np.minimum(sq1, sq2) < 0.165, 'ivory')
    put(np.minimum(sq1, sq2) < 0.155, 'navy')
    put(np.maximum(sq1, sq2) < 0.1, 'gold')
    oc0 = np.maximum(np.maximum(ax_, ay_), (ax_ + ay_) * 0.72)
    put(oc0 < 0.055, 'redd')
    put(oc0 < 0.034, 'ivory')
    put(oc0 < 0.014, 'navy')
    for sx in (-1, 1):
        pd = np.abs(X - sx * 0.67) / 0.1 + ay_ / 0.075
        put(pd < 1.1, 'ivory')
        put(pd < 1.0, 'navy')
        put(pd < 0.55, 'gold')
        put(pd < 0.25, 'redd')
        put((ay_ < 0.007) & (sx * X > 0.5) & (sx * X < 0.58), 'ivory')
    for sy in (-1, 1):
        pd = ax_ / 0.06 + np.abs(Y - sy * 0.425) / 0.05
        put(pd < 1.12, 'ivory')
        put(pd < 1.0, 'navy')
        put(pd < 0.45, 'gold')

    # ---- hooked spandrels in the field's corners: navy, a gold line, a blue heart, three flowers, a corner rosette
    for sx in (-1, 1):
        for sy in (-1, 1):
            dxs, dys = np.abs(X - sx * (hw - 0.23)), np.abs(Y - sy * (hh - 0.23))
            qd = dxs / 0.5 + dys / 0.36
            qh = qd - 0.045 * (((dxs / 0.5 - dys / 0.36) * 3 + 0.25) % 1 < 0.5)
            put(field & (qh < 1.05), 'ivory')
            put(field & (qh < 1.0), 'navy')
            put(field & (np.abs(qd - 0.66) < 0.022), 'gold')
            put(field & (qd < 0.5), 'ivory')
            put(field & (qd < 0.47), 'blue')
            for t_ in (0.18, 0.5, 0.82):
                fdx, fdy = np.abs(dxs - 0.84 * 0.5 * (1 - t_)), np.abs(dys - 0.84 * 0.36 * t_)
                fo = np.maximum(np.maximum(fdx, fdy), (fdx + fdy) * 0.72)
                put(field & (fo < 0.024), 'ivory')
                put(field & (fo < 0.017), 'gold' if t_ != 0.5 else 'red')
                put(field & (fo < 0.006), 'navy')
            ocs = np.maximum(np.maximum(dxs, dys), (dxs + dys) * 0.72)
            put(field & (ocs < 0.08), 'ivory')
            put(field & (ocs < 0.07), 'redd')
            put(field & (ocs < 0.035), 'gold')

    # ---- the borders
    border = ~field
    longside = (hh - ay_) < (hw - ax_)
    al = np.where(longside, X, Y)
    kil = border & ~longside & (edge < 0.016)
    bind = border & longside & (edge < 0.008)
    put(border & (edge < 0.016), 'ivory')
    put(bind, 'redd')
    put(kil & (edge > 0.006) & (edge < 0.0095), 'redd')
    # the outer guard: a red sawtooth on ivory
    tt = _tri(al / 0.024)
    put(border & (edge >= 0.016) & (edge < 0.036), 'ivory')
    put(border & (edge >= 0.016) & (edge < 0.036) & ((edge - 0.016) / 0.02 < tt), 'redd')
    put(border & (edge >= 0.036) & (edge < 0.042), 'navy')
    # the main border: navy, stepped rosettes on a zigzag vine; a rosette in each corner square
    mb = border & (edge >= 0.042) & (edge < 0.19)
    put(mb, 'navy')
    run_l, run_s = hw - 0.19, hh - 0.19
    P_l, P_s = 2 * run_l / 12, 2 * run_s / 8
    per = np.where(longside, P_l, P_s)
    run = np.where(longside, run_l, run_s)
    k = np.clip(np.floor((al + run) / per), 0, np.where(longside, 11, 7))
    da = al - (-run + (k + 0.5) * per)
    de = edge - 0.116
    corner = (hw - ax_ < 0.19) & (hh - ay_ < 0.19)
    da = np.where(corner, ax_ - (hw - 0.116), da)
    de = np.where(corner, ay_ - (hh - 0.116), de)
    ada, ade = np.abs(da), np.abs(de)
    ro = np.maximum(np.maximum(ada, ade), (ada + ade) * 0.72)
    zig = 0.032 * (2 * _tri(al / per * 2 + 0.5) - 1)
    put(mb & ~corner & (np.abs(de - zig) < 0.0065) & (ro > 0.064), 'gold')
    leaf = np.abs(de - zig * 1.35) / 0.012 + np.abs(((al / per * 2 + 0.5) % 1) - 0.5) * per / 0.024
    put(mb & ~corner & (leaf < 1) & (ro > 0.066) & (np.abs(de) > 0.034), 'blue')
    put(mb & (ro < 0.058), 'ivory')
    put(mb & (ro < 0.052), 'red')
    put(mb & (((ada < 0.011) & (ade < 0.04)) | ((ade < 0.011) & (ada < 0.04))), 'gold')
    put(mb & (np.hypot(ada - 0.025, ade - 0.025) < 0.008), 'navy')
    put(mb & (ro < 0.012), 'ivory')
    put(border & (edge >= 0.19) & (edge < 0.196), 'ivory')
    # the inner guard: a chain of ivory diamonds on dark madder
    ig = border & (edge >= 0.196) & (edge < 0.222)
    put(ig, 'redd')
    t2 = np.abs((al / 0.022) % 1 - 0.5)
    put(ig & (t2 / 0.42 + np.abs(edge - 0.209) / 0.0095 < 1), 'ivory')
    put(ig & (t2 / 0.14 + np.abs(edge - 0.209) / 0.003 < 1), 'gold')
    put(border & (edge >= 0.222), 'navy')

    # ---- dark outlines where colors meet, the pile's softness
    lum = col.mean(axis=2)
    gm = np.abs(np.roll(lum, 1, 1) - lum) + np.abs(np.roll(lum, 1, 0) - lum)
    outline = np.clip(gm * 10.0, 0, 1)
    put(outline * 0.4, 'brown')
    col = blur(col, 1)
    # fibre and tufts (fine), mottling (coarse), abrash bands across the rug
    fib = _vn2(X0, Y0, 0.0016, seed=3, octaves=2)
    tuft = _vn2(X0, Y0, 0.0045, seed=5, octaves=1)
    mott = _vn2(X0, Y0, 0.06, seed=6, octaves=3)
    col = col * (0.84 + 0.2 * fib[..., None] + 0.08 * tuft[..., None]) * (0.94 + 0.12 * mott[..., None])
    ab = _vn2(X0, np.zeros_like(Y0), 0.11, seed=7, octaves=2)
    col = col * (0.9 + 0.2 * ab[..., None]) * (1.0 + (ab[..., None] - 0.5) * np.array([0.06, 0.0, -0.08]))
    # the sun through the window has faded the +X end
    grey = col.mean(axis=2, keepdims=True)
    sun = (np.clip((X0 - 0.1) / 1.1, 0, 1) ** 1.3 * 0.24)[..., None]
    col = col * (1 - sun) + (grey * 0.9 + _lin(0xb5a68a) * 0.1) * sun
    # the traffic path: in at the -X end and across the front (-Y), hardest where feet turn
    yc_ = -0.33 - 0.12 * np.sin(X0 * 1.4 + 0.5)
    path = np.exp(-((Y0 - yc_) / 0.26) ** 2) * np.clip((0.95 - X0) / 1.4, 0.2, 1.0)
    path = path * (0.62 + 0.38 * _vn2(X0, Y0, 0.07, seed=10, octaves=3))
    worn = np.clip(path * 1.15 - 0.15, 0, 1)
    w3 = worn[..., None]
    grey = col.mean(axis=2, keepdims=True)
    col = col * (1 - 0.42 * w3) + (grey * 1.02 + 0.006) * 0.42 * w3
    col = col * (1 - 0.2 * w3) + _lin(0x5a4c3c) * 0.2 * w3
    # worn to the foundation where it's worst: the cotton warp and weft, the knots' roots in rows
    bald = (np.clip((worn - 0.8) * 5.0, 0, 1) * np.clip((_vn2(X0, Y0, 0.02, seed=15, octaves=2) - 0.55) * 5.0, 0, 1)
            * np.clip((-0.3 - X0) / 0.4, 0, 1))
    warp = 0.5 + 0.5 * np.cos(Y0 / 0.0045 * 2 * np.pi)
    weft = 0.5 + 0.5 * np.cos(X0 / 0.005 * 2 * np.pi)
    found = _lin(0xb0a285) * (0.78 + 0.22 * warp[..., None]) * (0.85 + 0.15 * weft[..., None])
    found = found * 0.72 + col * 0.28 * (0.6 + 0.4 * weft[..., None])
    b3 = bald[..., None]
    col = col * (1 - b3) + found * b3
    # a coffee stain with its tide line, two old spots
    st = np.hypot((X0 - 0.48) / 0.1, (Y0 + 0.36) / 0.075) + 0.35 * (_vn2(X0, Y0, 0.03, seed=13, octaves=2) - 0.5)
    stain = (np.clip((1.0 - st) * 6, 0, 1) * 0.38 + np.exp(-((st - 0.97) / 0.06) ** 2) * 0.5)[..., None]
    col = col * (1 - stain * 0.6) + _lin(0x3c2612) * stain * 0.6
    for sx, sy, rr, sd in ((-0.62, 0.31, 0.025, 51), (0.86, 0.12, 0.018, 52)):
        sp = np.hypot(X0 - sx, Y0 - sy) / rr + 0.4 * (_vn2(X0, Y0, 0.01, seed=sd) - 0.5)
        m_ = (np.clip((1 - sp) * 4, 0, 1) * 0.35)[..., None]
        col = col * (1 - m_) + _lin(0x2a1c12) * m_
    # grime where shoes go round the edges, heavier at the front; a film of dust over everything
    edge0 = np.minimum(hw - np.abs(X0), hh - np.abs(Y0))
    grime = np.clip(1 - edge0 / 0.14, 0, 1) * (0.5 + 0.5 * _vn2(X0, Y0, 0.08, seed=14)) * (0.18 + 0.12 * (Y0 < 0))
    col = col * (1 - grime[..., None])
    col = col * 0.93 + _lin(0x5e5448) * 0.05
    # the surface: pile height (fibre, tufts, a little lower at the outlines; crushed in the path; the foundation
    # where it's bald; the flatweave ends low, the binding round) and wear
    hgt = 0.68 + 0.18 * fib + 0.1 * tuft - 0.08 * outline - 0.3 * worn
    hgt = hgt * (1 - bald) + (0.1 + 0.14 * warp * weft) * bald
    hgt = np.where(kil, 0.36 + 0.1 * weft, hgt)
    hgt = np.where(bind, 0.8, hgt)
    surf = np.stack([np.clip(hgt, 0, 1), np.clip(worn + 0.5 * bald, 0, 1), np.zeros_like(hgt)], axis=2)
    cp = save_img('dr_rug_color', np.clip(col, 0, 1))
    hp = save_img('dr_rug_surface', surf, 'Non-Color')
    return cp, hp


def rug_curl(P):
    """Rolls one corner of the rug up off the floor (numpy points, in place copy)."""
    P = np.array(P, float)
    C = np.array(R_CURL['corner'])
    d = np.array([-1.0, 1.0]) / math.sqrt(2.0)
    S0, R, tm = R_CURL['fold'], R_CURL['radius'], R_CURL['theta']
    rel = P[:, :2] - C
    s = rel @ d
    perp = rel - np.outer(s, d)
    t = S0 - s
    m = t > 0
    th = np.minimum(t[m] / R, tm)
    extra = np.maximum(t[m] - R * tm, 0.0)
    s_new = S0 - R * np.sin(th) - extra * np.cos(tm)
    z_add = R * (1 - np.cos(th)) + extra * np.sin(tm)
    newxy = C + np.outer(s_new, d) + perp[m]
    P[m, 0], P[m, 1] = newxy[:, 0], newxy[:, 1]
    # the pile's thickness turns with the roll
    ang = np.where(t[m] > R * tm, tm, t[m] / R)
    P[m, 2] = z_add + P[m, 2] * np.cos(ang)
    return P


# The buckle across the -X end: where it runs (x = R_BUCKLE[0] + R_BUCKLE[1] * y) and its height at the front.
R_BUCKLE = (-0.78, 0.07, 0.011)


def rug_lift(x, y):
    """How far the rug stands off the floor besides the curl: a buckle across the -X end, kicked up toward the front
    (-Y) and pinned flat toward the back (+Y, under the couch's front feet), and a millimeter or two of unevenness
    (none at the ends, which lie flat)."""
    x, y = np.asarray(x, float), np.asarray(y, float)
    d = x - (R_BUCKLE[0] + R_BUCKLE[1] * y)
    amp = R_BUCKLE[2] * np.clip((0.35 - y) / 0.7, 0, 1) ** 1.2
    buckle = amp * np.exp(-(d / 0.05) ** 2)
    lumps = 0.0018 * _vn2(x, y + 4.0, 0.35, seed=61, octaves=2) * np.clip((R_W / 2 - np.abs(x)) / 0.12, 0, 1)
    return buckle + lumps


def rug_form(P):
    """The rug's shape: the curled corner, then the buckle and the unevenness (by the flat position)."""
    P = np.asarray(P, float)
    Q = rug_curl(P)
    Q[:, 2] += rug_lift(P[:, 0], P[:, 1])
    return Q


def _spaced(values, min_gap=0.004):
    """Sorted grid coordinates with near-duplicates dropped (the ends kept)."""
    v = np.unique(np.round(np.asarray(values, float), 6))
    keep = [v[0]]
    for a in v[1:-1]:
        if a - keep[-1] >= min_gap and v[-1] - a >= min_gap:
            keep.append(a)
    keep.append(v[-1])
    return np.array(keep)


def build_rug():
    """SM_DR_Rug: 2.40 x 1.70 m, 8 mm pile (top at 0.008). A worn Heriz-style machine-made rug (rug_pattern): a
    traffic path worn to the foundation in places, a coffee stain, the +X end sun-faded; bound long edges, an ivory
    flatweave and knotted fringe at the short ends; the +X/-Y corner curled up (its tip about 11 cm off the floor)
    and a soft buckle across the -X end (R_BUCKLE: up to 1.1 cm at the front, flat by y +0.35). The underside shows
    the pattern coarsely, as a machine-made rug's does."""
    cp, hp = rug_pattern()
    m = core.Mat('drr_pile')
    uvp = uv_coords(m)
    img = m.image(cp, vector=uvp)
    hi = m.image(hp, non_color=True, vector=uvp)
    sc_ = m.node('ShaderNodeSeparateColor')
    m.link(hi.outputs['Color'], sc_.inputs['Color'])
    hgt, worn = sc_.outputs['Red'], sc_.outputs['Green']
    lay = m.node('ShaderNodeSeparateXYZ')
    m.link(uv_coords(m, 'UVMap'), lay.inputs['Vector'])
    back = ramp(m, lay.outputs['Y'], 0.805, 0.815)
    tc, sep = object_coords(m)
    jute = noise(m, scaled(m, sep, 900.0, 900.0, 900.0), detail=1.0)
    grid = m.math('MULTIPLY', m.math('ADD', m.math('SINE', m.math('MULTIPLY', sep.outputs['X'], 2 * math.pi / 0.003)), 1.0),
                  m.math('ADD', m.math('SINE', m.math('MULTIPLY', sep.outputs['Y'], 2 * math.pi / 0.003)), 1.0))
    backc = m.mix(0.55, img.outputs['Color'], H(0x6a5a44))
    backc = m.mix(m.math('MULTIPLY', grid, 0.15), backc, H(0x3a2e22))
    m.set('Base Color', m.mix(back, img.outputs['Color'], backc))
    # wool pile: matte, a touch less so where it's crushed flat in the path
    front_r = m.math('SUBTRACT', m.math('ADD', 0.88, m.math('MULTIPLY', hgt, 0.07)), m.math('MULTIPLY', worn, 0.1))
    m.set('Roughness', mix_float(m, back, front_r, 0.92))
    m.set('Sheen Weight', 0.5)
    m.set('Specular IOR Level', 0.25)
    set_bump(m, m.math('ADD', hgt, m.math('MULTIPLY', m.math('ADD', jute, grid), m.math('MULTIPLY', back, 0.3))), 0.6, 0.0015)
    fr = core.Mat('drr_fringe')
    tc2, sep2 = object_coords(fr)
    k = attr(fr, 'dr_k')
    dirt = ramp(fr, noise(fr, tc2.outputs['Object'], scale=30.0, detail=4.0), 0.35, 0.75)
    cotton = fr.mix(dirt, H(0xd9d0bc), H(0x8a7e68))
    fr.set('Base Color', fr.mix(is_k(fr, k, 1), cotton, H(0x5a1a16)))
    fr.set('Roughness', 0.9)
    set_bump(fr, noise(fr, tc2.outputs['Object'], scale=1500.0, detail=1.0), 0.3, 0.0004)
    # the pile: a grid, finer toward the curled corner and through the buckle, shaped, then given its thickness
    hw, hh = R_W / 2, R_H / 2
    xs = _spaced(np.concatenate([np.linspace(-hw, hw, 84), np.arange(-1.0, -0.55, 0.012), np.linspace(hw - 0.25, hw, 23)]))
    ys = _spaced(np.concatenate([np.linspace(-hh, hh, 60), np.linspace(-hh, -hh + 0.25, 23)]))
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    vs = [[bm.verts.new((x, y, R_T)) for x in xs] for y in ys]
    for j in range(len(ys) - 1):
        for i in range(len(xs) - 1):
            f = bm.faces.new((vs[j][i], vs[j][i + 1], vs[j + 1][i + 1], vs[j + 1][i]))
            for l in f.loops:
                l[uvl].uv = (l.vert.co.x / R_W + 0.5, l.vert.co.y / R_H + 0.5)
    top = core.mesh_object('rug', bm)
    set_verts(top, rug_form(verts_np(top)))
    top.data.shade_smooth()
    thicken(top, R_T - 0.0005)
    core.assign(top, m)
    out = [top]
    # bound long edges
    for sy in (-1, 1):
        pts = np.array([(x, sy * (R_H / 2 - 0.002), 0.0045) for x in np.linspace(-R_W / 2 + 0.004, R_W / 2 - 0.004, 200)])
        b_ = tube([tuple(p) for p in rug_form(pts)], 0.0048, 8, 1, 'binding')
        core.assign(b_, fr)
        kind(b_, 1)
        out.append(b_)
    # fringes: cotton warp ends knotted in fours, lying out over the floor
    rng = random.Random(6)
    for sx in (-1, 1):
        n = 76
        for g in range(n // 4):
            y0 = -R_H / 2 + 0.02 + (R_H - 0.04) * (g + 0.5) / (n // 4)
            knot = rug_form(np.array([(sx * (R_W / 2 + 0.006), y0, 0.004)]))[0]
            kn = parts.rbox('knot', tuple(knot), (0.012, 0.01, 0.007), 0.003)
            core.assign(kn, fr)
            kind(kn, 0)
            out.append(kn)
            for s_ in range(4):
                yy = y0 + (s_ - 1.5) * 0.004
                L = rng.uniform(0.055, 0.075)
                ang = rng.uniform(-0.25, 0.25)
                pts = []
                for t in np.linspace(0, 1, 6):
                    pts.append((sx * (R_W / 2 + 0.008 + L * t), yy + math.sin(ang) * L * t + 0.004 * math.sin(t * 5 + g + s_), 0.0014 + 0.003 * (1 - t) ** 2))
                pts = rug_form(np.array(pts))
                st_ = tube([tuple(p) for p in pts], 0.0012, 5, 2, 'fringe')
                core.assign(st_, fr)
                kind(st_, 0)
                out.append(st_)
    for o in out:
        if o.data.attributes.get('dr_k') is None:
            kind(o, 0)
    return [finish(out, 'SM_DR_Rug', [m, fr], angle=60.0, weighted=False)]


# ================================================================== SM_DR_FloorLamp

L_SOCKET = 1.30
L_SHADE = (1.235, 1.535, 0.228, 0.132)   # bottom z, top z, bottom radius, top radius
L_BULB = (0.0, 0.0, 1.37)
L_TILT = 3.5


def shell(name, rings):
    """A closed shell through rings (each a closed loop of the same count), the last joined back to the first."""
    bm = bmesh.new()
    vs = [[bm.verts.new(tuple(p)) for p in r] for r in rings]
    m = len(rings[0])
    for a, b in zip(vs, vs[1:] + vs[:1]):
        for k in range(m):
            bm.faces.new((a[k], a[(k + 1) % m], b[(k + 1) % m], b[k]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    obj = core.mesh_object(name, bm)
    obj.data.shade_smooth()
    return obj


def brass(name, hex_color=0x8e6e32, tarnish=0.8, rough=0.38):
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    patch = ramp(m, noise(m, o, scale=14.0, detail=5.0), 0.4, 0.7)
    cav = cavity(m, 0.01, 1.6)
    t = m.math('MULTIPLY', m.math('MAXIMUM', patch, cav), tarnish)
    rub = ramp(m, convex_edges(m, tc, distance=0.002, samples=12, breakup_scale=80.0, gain=4.0), 0.2, 0.6)
    col = m.mix(t, H(hex_color), H(0x4a3a1c))
    col = m.mix(rub, col, H(0xd8b868))
    m.set('Base Color', col)
    m.set('Metallic', mix_float(m, t, 1.0, 0.7))
    m.set('Roughness', mix_float(m, rub, m.math('ADD', rough, m.math('MULTIPLY', t, 0.3)), 0.2))
    return m


def build_lamp():
    """SM_DR_FloorLamp: 1.56 m to the finial: a weighted brass dome base (0.30), a two-piece pole, a three-way socket
    with two pull chains, a harp, and a pleated ivory shade (0.46 across the bottom) knocked a few degrees askew,
    scorched near the top. The shade's outside glows in its baked emissive (slot drl_shade, strength 5);
    SM_DR_FloorLamp_Glow is the bulb and the shade's lining for the game to light. The bulb's middle is L_BULB; the
    cord runs back (+Y) to the wall with a foot switch on it."""
    br = brass('drl_brass')
    sh = core.Mat('drl_shade')
    tc, sep = object_coords(sh)
    o = tc.outputs['Object']
    z = sep.outputs['Z']
    ang = sh.math('ARCTAN2', sep.outputs['Y'], sep.outputs['X'])
    pleat = sh.math('ABSOLUTE', sh.math('SUBTRACT', sh.math('FRACT', sh.math('MULTIPLY', ang, 56 / (2 * math.pi))), 0.5))
    weave = noise(sh, scaled(sh, sep, 1500.0, 1500.0, 1500.0), detail=1.0)
    yellow = ramp(sh, z, L_SHADE[0] + 0.05, L_SHADE[1])
    scorch_d = sh.math('ADD', dist3(sh, sep, (0.11 * math.cos(0.6), 0.11 * math.sin(0.6), L_SHADE[1] - 0.04)),
                       sh.math('MULTIPLY', noise(sh, o, scale=80.0, detail=3.0), 0.02))
    scorch = smooth_less(sh, scorch_d, 0.04, 0.02)
    base = sh.mix(sh.math('MULTIPLY', yellow, 0.6), H(0xdcceac), H(0xc9b07a))
    base = sh.mix(sh.math('MULTIPLY', pleat, 0.25), base, H(0xb8a882))
    base = sh.mix(sh.math('MULTIPLY', scorch, 0.7), base, H(0x8a5a2a))
    sh.set('Base Color', base)
    # the light through the fabric: brightest at the bulb's height, the trims and frame dark, the pleats' folds
    zb = sh.math('DIVIDE', sh.math('SUBTRACT', z, L_BULB[2]), 0.14)
    lit = sh.math('EXPONENT', sh.math('MULTIPLY', sh.math('MULTIPLY', zb, zb), -1.0))
    lit = sh.math('ADD', 0.45, sh.math('MULTIPLY', lit, 0.55))
    lit = sh.math('MULTIPLY', lit, sh.math('ADD', 0.82, sh.math('MULTIPLY', pleat, 0.36)))
    lit = sh.math('MULTIPLY', lit, sh.math('ADD', 0.9, sh.math('MULTIPLY', weave, 0.2)))
    lit = sh.math('MULTIPLY', lit, sh.math('SUBTRACT', 1.0, sh.math('MULTIPLY', scorch, 0.6)))
    trims = sh.math('MAXIMUM', smooth_less(sh, sh.math('SUBTRACT', z, L_SHADE[0]), 0.012, 0.003),
                    smooth_less(sh, sh.math('SUBTRACT', L_SHADE[1], z), 0.012, 0.003))
    lit = sh.math('MULTIPLY', lit, sh.math('SUBTRACT', 1.0, sh.math('MULTIPLY', trims, 0.85)))
    seam = smooth_less(sh, sh.math('ABSOLUTE', sh.math('SUBTRACT', ang, 2.2)), 0.012, 0.004)
    lit = sh.math('MULTIPLY', lit, sh.math('SUBTRACT', 1.0, sh.math('MULTIPLY', seam, 0.4)))
    sh.set('Emission Color', sh.mix(1.0, H(0xffc68a), vec(sh, lit, lit, lit), blend='MULTIPLY'))
    sh.set('Emission Strength', 5.0)
    sh.set('Roughness', 0.9)
    sh.set('Sheen Weight', 0.4)
    set_bump(sh, sh.math('ADD', weave, sh.math('MULTIPLY', pleat, 2.0)), 0.2, 0.0008)
    cd = core.Mat('drl_cord')
    tc3, sep3 = object_coords(cd)
    k = attr(cd, 'dr_k')
    cd.set('Base Color', cd.mix(is_k(cd, k, 1), H(0x3b2a1c), H(0x0f0f0f)))
    cd.set('Roughness', cd.math('ADD', 0.45, cd.math('MULTIPLY', noise(cd, tc3.outputs['Object'], scale=400.0), 0.1)))
    out = []

    def add(o, m, kk=0):
        core.assign(o, m)
        kind(o, kk)
        out.append(o)
        return o

    add(lathe('base', [(0.0, 0.0), (0.148, 0.0), (0.151, 0.005), (0.15, 0.012), (0.142, 0.017), (0.115, 0.03), (0.075, 0.044), (0.045, 0.052),
                       (0.028, 0.058), (0.022, 0.07), (0.016, 0.075), (0.0, 0.075)], 72), br)
    add(rod((0, 0, 0.07), (0, 0, 0.68), 0.0125, 24, 'pole'), br)
    add(lathe('coupling', [(0.0, 0.66), (0.016, 0.66), (0.0175, 0.67), (0.0175, 0.69), (0.016, 0.7), (0.0, 0.7)], 24), br)
    add(rod((0, 0, 0.69), (0, 0, 1.2), 0.0115, 24, 'pole2'), br)
    add(lathe('knuckle', [(0.0, 1.06), (0.013, 1.06), (0.02, 1.075), (0.021, 1.09), (0.014, 1.105), (0.0, 1.105)], 24), br)
    add(lathe('neck', [(0.0, 1.195), (0.013, 1.195), (0.016, 1.22), (0.011, 1.25), (0.011, L_SOCKET - 0.06), (0.0, L_SOCKET - 0.06)], 24), br)
    # the socket shell and its key, the two pull chains
    add(lathe('socket', [(0.0, L_SOCKET - 0.062), (0.014, L_SOCKET - 0.062), (0.019, L_SOCKET - 0.05), (0.02, L_SOCKET - 0.01), (0.017, L_SOCKET),
                         (0.0, L_SOCKET)], 32), br)
    add(parts.disc('key', (0.024, 0.0, L_SOCKET - 0.035), (1, 0, 0), 0.0055, 0.012, 16), br)
    for i, (dx, dy, ln) in enumerate(((-0.012, -0.012, 0.13), (0.01, -0.016, 0.1))):
        p0 = Vector((dx, dy, L_SOCKET - 0.06))
        pts = [p0, p0 + Vector((dx * 0.2, -0.003, -ln * 0.5)), p0 + Vector((dx * 0.35, -0.006 * (i + 1), -ln))]
        pts = core.catmull_rom(pts, 10)
        add(sweep_var('chain', pts, lambda t, a, ln=ln: 0.0011 * (1.0 + 0.45 * math.cos(t * ln / 0.0032 * 2 * math.pi)), 6), br)
        add(lathe('pull', [(0.0, 0.0), (0.0035, 0.0), (0.005, 0.008), (0.0035, 0.016), (0.0, 0.017)], 12,
                  Matrix.Translation(Vector(pts[-1]) - Vector((0, 0, 0.017)))), br)
    # the harp, the spider and the finial, the shade: all knocked askew about the socket
    tilt = Matrix.Translation((0, 0, L_SOCKET - 0.06)) @ Matrix.Rotation(math.radians(L_TILT), 4, Vector((0.6, 1.0, 0.0)).normalized()) @ Matrix.Translation((0, 0, -(L_SOCKET - 0.06)))
    harp_pts = [(-0.06, 0.0, L_SOCKET - 0.045), (-0.075, 0.0, L_SOCKET + 0.06), (-0.07, 0.0, 1.45), (-0.035, 0.0, 1.505), (0.0, 0.0, 1.515),
                (0.035, 0.0, 1.505), (0.07, 0.0, 1.45), (0.075, 0.0, L_SOCKET + 0.06), (0.06, 0.0, L_SOCKET - 0.045)]
    add(xf(tube(harp_pts, 0.0024, 10, 6, 'harp'), tilt), br)
    add(xf(rbox('saddle', (0, 0, L_SOCKET - 0.05), (0.13, 0.012, 0.005), 0.002), tilt), br)
    zt = L_SHADE[1] - 0.012
    for a in (0.0, 2 * math.pi / 3, 4 * math.pi / 3):
        add(xf(rod((0.0, 0.0, zt), (math.cos(a) * (L_SHADE[3] - 0.004), math.sin(a) * (L_SHADE[3] - 0.004), zt), 0.0016, 8, 'spider'), tilt), br)
    add(xf(lathe('washer', [(0.0, zt - 0.002), (0.018, zt - 0.002), (0.018, zt + 0.002), (0.0, zt + 0.002)], 24), tilt), br)
    add(xf(lathe('finial', [(0.0, zt + 0.002), (0.008, zt + 0.002), (0.006, zt + 0.012), (0.014, zt + 0.022), (0.012, zt + 0.032), (0.004, zt + 0.04),
                            (0.0, zt + 0.042)], 24), tilt), br)
    # the pleated shade: a thin shell, its pleats zigzagging round
    NP = 56
    z0, z1, r0, r1 = L_SHADE

    def ring(zz, inset):
        tt = (zz - z0) / (z1 - z0)
        R = r0 + (r1 - r0) * tt
        a_ = 0.0062 - 0.0022 * tt
        pts = []
        for i in range(NP * 2):
            th = 2 * math.pi * i / (NP * 2)
            rr = R + (a_ if i % 2 == 0 else -a_) - inset
            pts.append((rr * math.cos(th), rr * math.sin(th), zz))
        return pts
    zs = list(np.linspace(z0, z1, 9))
    rings = [ring(zz, 0.0) for zz in zs] + [ring(zz, 0.0016) for zz in reversed(zs)]
    shade = shell('shade', rings)
    add(xf(shade, tilt), sh)
    for zz, rr in ((z0, r0), (z1, r1)):
        trim = [(math.cos(2 * math.pi * i / 96) * (rr + 0.0065), math.sin(2 * math.pi * i / 96) * (rr + 0.0065), zz) for i in range(96)]
        add(xf(core.sweep('trim', [Vector(p) for p in trim], parts.circle(0.0042, 8), closed=True), tilt), cd, 1)
    # the cord: out of the base at the back, to the wall, a foot switch on the way
    cord_pts = [(0.0, 0.14, 0.003), (0.01, 0.2, 0.003), (0.04, 0.28, 0.003), (0.0, 0.34, 0.003), (-0.12, 0.37, 0.003), (-0.3, 0.38, 0.003)]
    add(tube(cord_pts, 0.0028, 8, 10, 'cord'), cd, 0)
    add(rbox('footswitch', (0.035, 0.28, 0.012), (0.04, 0.065, 0.024), 0.008), cd, 1)
    add(rbox('switchbtn', (0.035, 0.285, 0.026), (0.026, 0.026, 0.006), 0.004), cd, 1)
    lamp = finish(out, 'SM_DR_FloorLamp', [br, sh, cd], angle=40.0)
    # the glow: the bulb, and the shade's lining a millimetre inside it
    bulb = lathe('bulb', [(0.0, -0.06), (0.0125, -0.06), (0.014, -0.045), (0.022, -0.02), (0.03, 0.005), (0.031, 0.02), (0.026, 0.038),
                          (0.015, 0.05), (0.0, 0.054)], 32, Matrix.Translation(L_BULB))
    lining = shell('lining', [ring(zz, 0.0026) for zz in zs] + [ring(zz, 0.0036) for zz in reversed(zs)])
    xf(lining, tilt)
    xf(bulb, tilt)
    g = glow([bulb, lining], 'SM_DR_FloorLamp_Glow')
    return [lamp, g]


# ================================================================== SM_DR_Radiator

H_N, H_PITCH = 14, 0.064
H_TOP = 0.66


def radiator_paint():
    """Silver radiator paint over cream over a green from long ago, over cast iron: chipped through the layers on the
    edges, drips frozen down the columns, dust in every crevice, rust where it meets the floor and the valve."""
    m = core.Mat('drh_paint')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    z = sep.outputs['Z']
    edge = convex_edges(m, tc, distance=0.004, samples=12, breakup_scale=45.0, gain=6.0)
    blot = noise(m, o, scale=26.0, detail=5.0, roughness=0.65)
    chip1 = m.math('MAXIMUM', ramp(m, m.math('MULTIPLY', edge, ramp(m, blot, 0.25, 0.55)), 0.25, 0.4), ramp(m, blot, 0.68, 0.72))
    chip2 = m.math('MULTIPLY', chip1, ramp(m, noise(m, o, scale=40.0, detail=4.0), 0.5, 0.58))
    chip3 = m.math('MULTIPLY', chip2, ramp(m, noise(m, o, scale=60.0, detail=4.0), 0.55, 0.62))
    sheen = noise(m, o, scale=300.0, detail=2.0)
    aged = ramp(m, noise(m, o, scale=4.0, detail=3.0), 0.3, 0.7)
    silver = m.mix(m.math('MULTIPLY', sheen, 0.4), H(0x8c8a80), H(0xa29e90))
    silver = m.mix(m.math('MULTIPLY', aged, 0.5), silver, H(0x8a7e62))
    col = m.mix(chip1, silver, H(0xd6ccb0))
    col = m.mix(chip2, col, H(0x5c7a5a))
    col = m.mix(chip3, col, H(0x4a2c18))
    metal = mix_float(m, chip1, 0.55, 0.0)
    rough = mix_float(m, chip1, m.math('ADD', 0.42, m.math('MULTIPLY', sheen, 0.1)), 0.6)
    rough = mix_float(m, chip3, rough, 0.85)
    # drips: thicker runs of paint down the columns
    drip = ramp(m, noise(m, scaled(m, sep, 220.0, 220.0, 6.0), detail=2.0), 0.66, 0.74)
    col = m.mix(m.math('MULTIPLY', drip, 0.25), col, H(0xc4c4bc))
    # dust and grime in the crevices, rust low down
    cav = cavity(m, 0.045, 2.2)
    col = m.mix(m.math('MULTIPLY', cav, 0.8), col, H(0x3e372c))
    rough = mix_float(m, cav, rough, 0.85)
    metal = mix_float(m, cav, metal, 0.1)
    low = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ramp(m, z, 0.04, 0.16)), ramp(m, noise(m, o, scale=30.0, detail=4.0), 0.35, 0.65))
    col = m.mix(m.math('MULTIPLY', low, 0.8), col, H(0x5a3018))
    rough = mix_float(m, low, rough, 0.9)
    metal = mix_float(m, low, metal, 0.0)
    dust_top = m.math('MULTIPLY', ramp(m, z, H_TOP - 0.03, H_TOP - 0.005), ramp(m, noise(m, o, scale=50.0, detail=3.0), 0.3, 0.7))
    col = m.mix(m.math('MULTIPLY', dust_top, 0.5), col, H(0x8a8476))
    m.set('Base Color', col)
    m.set('Metallic', metal)
    m.set('Roughness', rough)
    set_bump(m, m.math('SUBTRACT', m.math('ADD', m.math('MULTIPLY', drip, 0.6), noise(m, o, scale=500.0, detail=2.0)), m.math('MULTIPLY', chip1, 0.8)), 0.25, 0.0008)
    return m


def build_radiator():
    """SM_DR_Radiator: a 14-section three-column cast-iron radiator, 0.94 x 0.19 x 0.66 m with its valve (on the left,
    a black wheel, the supply pipe down into the floor) and an air vent (top right). Painted over many times. A dented
    aluminum cake pan of water on top, scaled with its old water lines."""
    pt = radiator_paint()
    pan_m = core.Mat('drh_pan')
    tc, sep = object_coords(pan_m)
    o = tc.outputs['Object']
    k = attr(pan_m, 'dr_k')
    alu = pan_m.mix(ramp(pan_m, noise(pan_m, o, scale=18.0, detail=4.0), 0.3, 0.7), H(0x9e9c96), H(0x6e665a))
    scale_ln = pan_m.math('MULTIPLY', smooth_less(pan_m, pan_m.math('ABSOLUTE', pan_m.math('SUBTRACT', pan_m.math('FRACT', pan_m.math('MULTIPLY', sep.outputs['Z'], 160.0)), 0.5)), 0.08, 0.04),
                          ramp(pan_m, sep.outputs['Z'], H_TOP + 0.012, H_TOP + 0.02))
    alu = pan_m.mix(pan_m.math('MULTIPLY', scale_ln, 0.6), alu, H(0xe4e0d4))
    water = pan_m.mix(ramp(pan_m, noise(pan_m, o, scale=40.0, detail=3.0), 0.4, 0.7), H(0x161612), H(0x1e1d18))
    wet = is_k(pan_m, k, 2)
    pan_m.set('Base Color', pan_m.mix(wet, alu, water))
    pan_m.set('Metallic', mix_float(pan_m, wet, 0.9, 0.0))
    pan_m.set('Roughness', mix_float(pan_m, wet, pan_m.math('ADD', 0.35, pan_m.math('MULTIPLY', scale_ln, 0.4)), 0.02))
    pan_m.set('Specular IOR Level', mix_float(pan_m, wet, 0.5, 0.6))
    vm = core.Mat('drh_valve')
    tc2, sep2 = object_coords(vm)
    k2 = attr(vm, 'dr_k')
    br_c = vm.mix(ramp(vm, noise(vm, tc2.outputs['Object'], scale=30.0, detail=4.0), 0.4, 0.7), H(0x9a7a3a), H(0x4a3a20))
    vm.set('Base Color', vm.mix(is_k(vm, k2, 1), br_c, H(0x121110)))
    vm.set('Metallic', mix_float(vm, is_k(vm, k2, 1), 0.9, 0.0))
    vm.set('Roughness', mix_float(vm, is_k(vm, k2, 1), 0.4, 0.3))
    out = []

    def add(o, m, kk=0):
        core.assign(o, m)
        kind(o, kk)
        out.append(o)
        return o

    zt, zb = 0.632, 0.112
    for i in range(H_N):
        x = (i - (H_N - 1) / 2) * H_PITCH
        for yc in (-0.058, 0.0, 0.058):
            add(soft_box('col', (0.021, 0.0165, (zt - zb) / 2), 0.0155, 0.07, None, Matrix.Translation((x, yc, (zt + zb) / 2)), arcs=(22.5,)), pt)
        for zz in (zt, zb):
            add(soft_box('head', (0.0305, 0.078, 0.036), 0.03, 0.03, None, Matrix.Translation((x, 0.0, zz + (0.008 if zz > 0.3 else -0.006))), arcs=(22.5,)), pt)
        if i in (0, H_N - 1):
            for yc in (-0.055, 0.055):
                add(soft_box('leg', (0.024, 0.02, 0.05), 0.012, 0.04, None, Matrix.Translation((x, yc, 0.05)), arcs=(22.5,)), pt)
    xe = (H_N - 1) / 2 * H_PITCH
    for zz in (zt, zb):
        add(rod((-xe, 0, zz), (xe, 0, zz), 0.021, 20, 'hub'), pt)
    # the valve on the left: a stub, the angle valve, its wheel, the pipe down through the floor
    vx = -xe - 0.03
    add(rod((-xe - 0.01, 0, zb), (vx - 0.03, 0, zb), 0.016, 20, 'stub'), pt)
    add(lathe('valve', [(0.0, -0.035), (0.016, -0.035), (0.024, -0.02), (0.026, 0.0), (0.022, 0.025), (0.012, 0.035), (0.0, 0.036)], 24,
              Matrix.Translation((vx - 0.045, 0.0, zb))), vm, 0)
    add(rod((vx - 0.045, 0, zb + 0.03), (vx - 0.045, 0, zb + 0.07), 0.006, 12, 'stem'), vm, 0)
    add(lathe('wheel', [(0.0, 0.0), (0.028, 0.0), (0.031, 0.004), (0.031, 0.012), (0.026, 0.016), (0.008, 0.017), (0.0, 0.022)], 16,
              Matrix.Translation((vx - 0.045, 0, zb + 0.068))), vm, 1)
    add(rod((vx - 0.045, 0, zb - 0.03), (vx - 0.045, 0, -0.02), 0.016, 20, 'pipe'), pt)
    add(lathe('escutcheon', [(0.0, 0.0), (0.042, 0.0), (0.04, 0.004), (0.02, 0.008), (0.0, 0.008)], 32, Matrix.Translation((vx - 0.045, 0, 0.0))), vm, 0)
    # the air vent, top right
    add(lathe('vent', [(0.0, 0.0), (0.009, 0.0), (0.011, 0.012), (0.01, 0.03), (0.006, 0.038), (0.0, 0.04)], 16,
              Matrix.Translation((xe + 0.018, 0.0, zt)) @ Matrix.Rotation(math.radians(-90), 4, 'Y')), vm, 0)
    # the pan of water on top, dented
    def rr(w, d, r, zz, n=6):
        return [(x_, y_, zz) for x_, y_ in core.rounded_rect(-w / 2, -d / 2, w / 2, d / 2, r, steps=n)]
    ztop = H_TOP + 0.0
    rings = [rr(0.30, 0.2, 0.025, ztop), rr(0.326, 0.226, 0.03, ztop + 0.05), rr(0.322, 0.222, 0.028, ztop + 0.05), rr(0.296, 0.196, 0.024, ztop + 0.0015)]
    pan = core.loft('pan', rings)
    P = verts_np(pan)
    dent = np.exp(-(((P[:, 0] - 0.09) / 0.04) ** 2 + ((P[:, 2] - ztop - 0.03) / 0.015) ** 2))
    P[:, 1] += 0.006 * dent * (P[:, 1] < 0)
    set_verts(pan, P)
    pan.data.transform(mat4((0.03, 0.0, 0.0), rz=-4.0))
    add(pan, pan_m, 1)
    add(xf(core.sweep('rim', [Vector(p) for p in rr(0.328, 0.228, 0.031, ztop + 0.0505, 8)], parts.circle(0.0022, 8), closed=True),
           mat4((0.03, 0.0, 0.0), rz=-4.0)), pan_m, 1)
    wat = parts.panel('water', core.rounded_rect(-0.155, -0.105, 0.155, 0.105, 0.026, steps=6), ztop + 0.0015, ztop + 0.029)
    xf(wat, mat4((0.03, 0.0, 0.0), rz=-4.0))
    add(wat, pan_m, 2)
    return [finish(out, 'SM_DR_Radiator', [pt, pan_m, vm], angle=40.0)]


# ================================================================== plants (shared)

def heart_leaf(size, fold, curl, n=8):
    """A pothos leaf: heart-shaped, its stem point at the origin, the blade along +Y facing +Z, folded at the midrib.
    'pat' u runs across (0..1), v along (0..1)."""
    edge = [(0.18, -0.08), (0.36, -0.06), (0.48, 0.05), (0.52, 0.2), (0.48, 0.38), (0.38, 0.56), (0.25, 0.73), (0.12, 0.88)]
    mids = [0.0, 0.06, 0.12, 0.2, 0.3, 0.42, 0.56, 0.72, 0.86, 1.0]
    bm = bmesh.new()

    def bend(x, y):
        return Vector((x * size, y * size, (abs(x) * fold - curl * (y - 0.45) ** 2) * size))

    mid = [bm.verts.new(bend(0.0, y)) for y in mids]
    pat = {v: (0.5, y) for v, y in zip(mid, mids)}
    for s in (-1, 1):
        e = [bm.verts.new(bend(s * x, y)) for x, y in edge]
        for v, (x, y) in zip(e, edge):
            pat[v] = (0.5 + s * x, y)
        bm.faces.new((mid[0], mid[1], e[0]) if s > 0 else (mid[1], mid[0], e[0]))
        for k in range(len(e) - 1):
            q = (mid[k + 1], mid[k + 2], e[k + 1], e[k])
            bm.faces.new(q if s > 0 else tuple(reversed(q)))
        bm.faces.new((mid[-2], mid[-1], e[-1]) if s > 0 else (mid[-1], mid[-2], e[-1]))
    uvl = bm.loops.layers.uv.new('UVMap')
    pl = bm.loops.layers.uv.new('pat')
    for f in bm.faces:
        for l in f.loops:
            l[pl].uv = pat[l.vert]
            l[uvl].uv = pat[l.vert]
    o = core.mesh_object('leaf', bm)
    o.data.uv_layers.active = o.data.uv_layers['UVMap']
    return o


def pothos(rng, base, plan, soil_z, pot_r, drop_edge=None):
    """Vines of a golden pothos from a pot at base (Vector): plan is a list of (angle degrees, kind, reach); kind 'up'
    stands, 'trail' runs out over the pot's rim and down (drop_edge: (y of a shelf's front edge, the drop) to hang
    over it), 'side' along the surface. Returns (vines, leaves, petioles)."""
    vines, leaves, pets = [], [], []
    for deg, kind_, reach in plan:
        a = math.radians(deg + rng.uniform(-8, 8))
        d = Vector((math.sin(a), -math.cos(a), 0.0))
        start = base + d * rng.uniform(0.0, pot_r * 0.4) + Vector((0, 0, soil_z))
        if kind_ == 'up':
            h = reach
            pts = [start, start + d * 0.01 + Vector((0, 0, h * 0.5)), start + d * rng.uniform(0.03, 0.07) + Vector((0, 0, h))]
        else:
            rim = base + d * (pot_r + 0.008) + Vector((0, 0, soil_z + 0.03))
            pts = [start, start + d * 0.03 + Vector((0, 0, 0.04)), rim, base + d * (pot_r + 0.03) + Vector((0, 0, soil_z - 0.02))]
            if kind_ == 'trail' and drop_edge is not None:
                ye, drop = drop_edge
                drop = drop or reach
                p = pts[-1]
                edge_p = Vector((p.x + d.x * 0.05, ye - 0.006, base.z + 0.003))
                pts += [Vector((p.x + d.x * 0.03, (p.y + ye) / 2, base.z + 0.006)), edge_p,
                        edge_p + Vector((d.x * 0.03, -0.02, -drop * 0.35)), edge_p + Vector((d.x * 0.06 + rng.uniform(-0.03, 0.03), -0.03, -drop * 0.7)),
                        edge_p + Vector((d.x * 0.08 + rng.uniform(-0.05, 0.05), -0.035 + rng.uniform(-0.01, 0.01), -drop))]
            else:
                p = pts[-1]
                pts += [p + d * reach * 0.4 + Vector((0, 0, -soil_z * 0.4)), p + d * reach + Vector((0, 0, -(p.z - base.z) + 0.004))]
        pts = core.catmull_rom([tuple(p) for p in pts], 8)
        vines.append(parts.tube('vine', pts, 0.0019, 6, 1))
        lengths = [0.0]
        for p0, p1 in zip(pts, pts[1:]):
            lengths.append(lengths[-1] + (Vector(p1) - Vector(p0)).length)
        total = lengths[-1]
        n = max(3, int(total / 0.05))
        for k in range(n):
            t = (k + 0.6) / n * total
            i = next(j for j in range(len(lengths) - 1) if lengths[j + 1] >= t)
            p = Vector(pts[i]).lerp(Vector(pts[i + 1]), (t - lengths[i]) / max(lengths[i + 1] - lengths[i], 1e-6))
            along = (Vector(pts[i + 1]) - Vector(pts[i])).normalized()
            size = (0.065 - 0.03 * k / max(n - 1, 1)) * rng.uniform(0.85, 1.12)
            lf = heart_leaf(size, rng.uniform(0.18, 0.32), rng.uniform(0.25, 0.5))
            side = 1 if k % 2 else -1
            out_dir = along.cross(Vector((0, 0, 1)))
            if out_dir.length < 1e-3:
                out_dir = Vector((1, 0, 0))
            out_dir = (out_dir.normalized() * side + along * 0.4 + Vector((0, -0.5, 0.35))).normalized()
            up = Vector((0, 0, 1)) - out_dir * out_dir.z
            up = (up + Vector((0, -0.6, 0))).normalized()
            x = out_dir.cross(up).normalized()
            z = x.cross(out_dir).normalized()
            m = Matrix((x, out_dir, z)).transposed().to_4x4()
            m = Matrix.Translation(p) @ m @ Matrix.Rotation(rng.uniform(-0.3, 0.3), 4, 'Y')
            base_p = p + out_dir * size * 0.3 + z * size * 0.06
            lf.data.transform(Matrix.Translation(base_p - p) @ m)
            leaves.append(lf)
            pets.append(parts.tube('petiole', [p, (p + base_p) / 2 + z * 0.003, base_p], 0.001, 5, 3))
    return vines, leaves, pets


def plant_material(name, pot_hex=0xb8653a, pot_kind='terracotta'):
    """The plants' slot: leaves (k 0, pothos gold-streaked green, 'pat' over each leaf), vines and petioles (k 1), soil
    (k 2), the pot (k 3: terracotta or glaze), its saucer (k 4)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    k = attr(m, 'dr_k')
    pu = m.node('ShaderNodeSeparateXYZ')
    m.link(uv_coords(m), pu.inputs['Vector'])
    streak = ramp(m, noise(m, scaled(m, sep, 140.0, 140.0, 40.0), detail=4.0, distortion=2.0), 0.58, 0.72)
    patch = noise(m, o, scale=35.0, detail=2.0)
    green = m.mix(patch, H(0x2a5520), H(0x48802c))
    vein = smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', pu.outputs['X'], 0.5)), 0.012, 0.006)
    leaf = m.mix(m.math('MULTIPLY', streak, 0.75), green, H(0xc8be6a))
    leaf = m.mix(m.math('MULTIPLY', vein, 0.5), leaf, H(0x8aa050))
    dust = m.math('MULTIPLY', ramp(m, noise(m, o, scale=60.0, detail=3.0), 0.4, 0.8), 0.25)
    leaf = m.mix(dust, leaf, H(0x8a8a78))
    vine = m.mix(patch, H(0x4a6e2a), H(0x6a8a3a))
    clumps = noise(m, o, scale=600.0, detail=5.0, roughness=0.7)
    soil = m.mix(clumps, H(0x17100a), H(0x2e2116))
    soil = m.mix(ramp(m, noise(m, o, scale=900.0, detail=1.0), 0.72, 0.75), soil, H(0xd8d4cc))
    if pot_kind == 'terracotta':
        pot = m.mix(ramp(m, noise(m, o, scale=20.0, detail=4.0), 0.3, 0.7), H(pot_hex), H(_shade(pot_hex, 0.8)))
        bloom = m.math('MULTIPLY', ramp(m, noise(m, o, scale=12.0, detail=4.0), 0.5, 0.7), 0.6)
        pot = m.mix(bloom, pot, H(0xd8d0c0))
        pot_r = 0.85
    else:
        drip = ramp(m, noise(m, scaled(m, sep, 40.0, 40.0, 4.0), detail=3.0), 0.4, 0.7)
        pot = m.mix(drip, H(pot_hex), H(_lighten(pot_hex, 0.3)))
        pot_r = 0.12
    col = m.mix(is_k(m, k, 1), leaf, vine)
    col = m.mix(is_k(m, k, 2), col, soil)
    col = m.mix(is_k(m, k, 3), col, pot)
    col = m.mix(is_k(m, k, 4), col, m.mix(0.5, pot, H(0x8a7a6a)))
    cact = m.mix(ramp(m, noise(m, o, scale=30.0, detail=3.0), 0.3, 0.7), H(0x24401f), H(0x36552c))
    cact = m.mix(m.math('MULTIPLY', ramp(m, noise(m, o, scale=90.0, detail=2.0), 0.55, 0.8), 0.35), cact, H(0x7a8a6c))
    col = m.mix(is_k(m, k, 5), col, H(0xddd4b0))
    col = m.mix(is_k(m, k, 6), col, cact)
    m.set('Base Color', col)
    rough = mix_float(m, is_k(m, k, 0), 0.5, m.math('ADD', 0.35, m.math('MULTIPLY', dust, 0.8)))
    rough = mix_float(m, is_k(m, k, 2), rough, 0.95)
    rough = mix_float(m, is_k(m, k, 3), rough, pot_r)
    m.set('Roughness', rough)
    m.set('Subsurface Weight', mix_float(m, is_k(m, k, 0), 0.0, 0.12))
    set_bump(m, m.math('ADD', m.math('MULTIPLY', clumps, is_k(m, k, 2)), m.math('MULTIPLY', patch, 0.2)), 0.4, 0.001)
    return m


# ================================================================== SM_DR_Shelf

S_W, S_D, S_H = 0.80, 0.30, 1.80
S_T = 0.019
S_LEVELS = [0.07, 0.41, 0.75, 1.09, 1.43]   # the shelves' undersides; their tops are + S_T
TITLES = [('THE LONG FOLD', 'R. MERCER'), ('RIVER OF LIES', 'J. ODELL'), ('DEAD MAN\'S HAND', 'T. CROSS'), ('THE QUIET TELL', 'A. VANCE'),
          ('SUMMER AT BLUE LAKE', 'M. HALLORAN'), ('NIGHT SHIFT', 'K. DORSEY'), ('HOLD \'EM BY NUMBERS', 'P. GRADY'), ('THE WIDOW\'S BET', 'L. FENN'),
          ('COLD DECK', 'S. BRAND'), ('A STRANGER IN TOWN', 'D. MARSH'), ('ALL THE WAY DOWN', 'C. ROWE'), ('LAST CALL', 'J. ODELL'),
          ('THE DEALER', 'R. MERCER'), ('HEARTS ON FIRE', 'M. HALLORAN'), ('FIVE CARD STUD', 'B. KEMP'), ('ROSES IN WINTER', 'E. LANE'),
          ('THE OUTSIDER', 'T. CROSS'), ('MIDNIGHT TRAIN', 'K. DORSEY'), ('SECOND CHANCES', 'E. LANE'), ('BURN CARD', 'S. BRAND'),
          ('THE HOUSE ALWAYS WINS', 'A. VANCE'), ('A GOOD MAN', 'D. MARSH'), ('DOUBLE DOWN', 'B. KEMP'), ('SHORT STACK', 'P. GRADY'),
          ('PAPER MOON', 'L. FENN'), ('THE RIVER CARD', 'J. ODELL'), ('NORTHSIDE', 'C. ROWE'), ('THE BLUFF', 'R. MERCER')]
SPINE_COLORS = [0x1a1a1a, 0x7a1414, 0x1c2a4a, 0xd8c8a0, 0x2a4a2a, 0x8a5a14, 0x4a1a3a, 0xc8b8a0, 0x101820, 0x9a2a1a, 0x3a3a3a, 0xe2d6b8]
CHIPS = [('RIVERSIDE', '5', 0xa8161e, 0xf2ede2), ('RIVERSIDE', '25', 0x1e6a34, 0xf2ede2), ('RIVERSIDE', '100', 0x161616, 0xf2ede2),
         ('RIVERSIDE', '500', 0x5a2a7a, 0xf2ede2), ('GOLDEN SPUR', '1', 0xe8e4d8, 0x1a3a7a), ('EMBERCREST', '25', 0xd85a1e, 0x1a1210),
         ('THE DOCKS', '1', 0x2a5aa8, 0xf2ede2), ('LUCKY SEVEN', '5', 0xc8a020, 0x6a1010)]


def chip_print(s, x, y, d, top, val, base, ink):
    r = d / 2
    cx, cy = x + r, y + r
    s.rect(x, y, d, d, base, rough=0.4)
    for k in range(6):
        a = k * math.pi / 3 + 0.26
        s.poly([(cx + r * math.cos(a - 0.17), cy + r * math.sin(a - 0.17)), (cx + r * math.cos(a + 0.17), cy + r * math.sin(a + 0.17)),
                (cx + r * 0.78 * math.cos(a + 0.15), cy + r * 0.78 * math.sin(a + 0.15)), (cx + r * 0.78 * math.cos(a - 0.15), cy + r * 0.78 * math.sin(a - 0.15))],
               ink, rough=0.4)
    s.ring(cx, cy, r * 0.62, r * 0.66, ink, rough=0.4)
    s.text(val, cx, cy - r * 0.2, r * (0.55 if len(val) < 3 else 0.42), ink, face='Black', align='CENTER', rough=0.4)
    s.text(top, cx, cy + r * 0.3, r * 0.15, ink, face='Bold', align='CENTER', rough=0.4)


def chip_edge(s, x, y, w, h, base, ink):
    s.rect(x, y, w, h, base, rough=0.4)
    for k in range(6):
        s.rect(x + (k + 0.25) * w / 6, y, w / 12, h, ink, rough=0.4)


def chip_obj(name, atlas, face_key, edge_key, r=0.0195, t=0.0033, matrix=None):
    """A clay chip: printed faces, its edge spots wrapping the rim."""
    o = parts.disc(name, (0, 0, 0), (0, 0, 1), r, t, 40)

    def fn(P, N):
        side = np.abs(N[:, 2]) < 0.5
        uv = atlas.uv(face_key, P[:, 0] / (2 * r) + 0.5, P[:, 1] / (2 * r) + 0.5)
        b = N[:, 2] < -0.5
        uv[b] = atlas.uv(face_key, P[b, 0] / (2 * r) + 0.5, 0.5 - P[b, 1] / (2 * r))
        ang = (np.arctan2(P[side, 1], P[side, 0]) / (2 * np.pi)) % 1.0
        uv[side] = atlas.uv(edge_key, ang, P[side, 2] / t + 0.5)
        return uv
    pat_faces(o, fn)
    if matrix is not None:
        o.data.transform(matrix)
    return o


def shelf_atlas(books):
    A = Atlas('drs_prints', 1000.0, 820.0, 2560)
    s = A.s
    A.fill('pages', 20, 12, 0xe2dac6, 0.7)
    for i, (title, author, w, h, color) in enumerate(books):
        x, y, ww, hh = A.region(f'spine{i}', w * 1000, h * 1000, gap=1.5)
        light = sum(((color >> sh) & 255) for sh in (16, 8, 0)) > 380
        ink = 0x1a1a1a if light else 0xeee6d2
        s.rect(x, y, ww, hh, color, rough=0.35)
        s.rect(x, y + hh - 14, ww, 1.2, ink, rough=0.35)
        s.rect(x, y + 12, ww, 1.2, ink, rough=0.35)
        s.text(title, x + ww / 2 + 2.2, y + hh / 2 + 10, min(6.2, ww * 0.42), ink, face='Black', align='CENTER', rot=-math.pi / 2, rough=0.35)
        s.text(author, x + ww / 2 + 1.6, y + 38, min(4.0, ww * 0.3), ink, face='Bold', align='CENTER', rot=-math.pi / 2, rough=0.35)
        s.circle(x + ww / 2, y + 6, min(3.0, ww * 0.3), ink, rough=0.35)
    for i, col in enumerate((0x1e3a2a, 0x5a1a1e, 0x1e2a44)):
        x, y, w, h = A.region(f'album{i}', 60, 300)
        s.rect(x, y, w, h, col, rough=0.5)
        s.rect(x + 4, y + h - 70, w - 8, 1, 0xc8a050, rough=0.3, metal=1.0)
        s.rect(x + 4, y + 40, w - 8, 1, 0xc8a050, rough=0.3, metal=1.0)
        s.text('PHOTOS', x + w / 2 + 4, y + h / 2 + 30, 11, 0xc8a050, face='Bold', align='CENTER', rot=-math.pi / 2, rough=0.3, metal=1.0)
        s.text(('1979-88', '1989-99', '2000-')[i], x + w / 2 + 3, y + 70, 8, 0xc8a050, face='Bold', align='CENTER', rot=-math.pi / 2, rough=0.3, metal=1.0)
    # photographs
    for key, kind_, w, h in (('ph_dealer', 'dealer', 102, 152), ('ph_beach', 'beach', 152, 102), ('ph_school', 'school', 89, 127),
                             ('ph_party', 'party', 152, 102), ('ph_wedding', 'wedding', 102, 127)):
        A.region(key, w, h)
        A.photo(key, photo(kind_, int(w * 4.8), int(h * 4.8), seed=len(key)), rough=0.25)
    # the plaque's engraved brass plate
    x, y, w, h = A.region('plate', 150, 105)
    s.rect(x, y, w, h, 0xb8954a, metal=1.0, rough=0.28)
    s.rect(x + 3, y + 3, w - 6, h - 6, 0x6a5020, metal=1.0, rough=0.4)
    s.rect(x + 4, y + 4, w - 8, h - 8, 0xc4a05a, metal=1.0, rough=0.26)
    ink = 0x241a0e
    s.text('RIVERSIDE CARD ROOM', x + w / 2, y + h - 18, 8.6, ink, face='Bold', align='CENTER', tracking=1.15, metal=0.0, rough=0.6)
    for k, st in enumerate('SHDC'):
        suit(s, st, x + w / 2 - 18 + k * 12, y + h - 30, 7, ink)
    s.text('BEST DEALER', x + w / 2, y + h - 52, 15, ink, face='Black', align='CENTER', metal=0.0, rough=0.6)
    s.text('1998', x + w / 2, y + 30, 14, ink, face='Black', align='CENTER', metal=0.0, rough=0.6)
    s.text('DEE', x + w / 2, y + 14, 9, ink, face='Bold', align='CENTER', tracking=1.4, metal=0.0, rough=0.6)
    # the chip frame's label, the chips, the dealer button
    x, y, w, h = A.region('frame_label', 110, 16)
    s.rect(x, y, w, h, 0xb8954a, metal=1.0, rough=0.3)
    s.text('THE RIVERSIDE · DEALERS', x + w / 2, y + 5, 6, 0x241a0e, face='Bold', align='CENTER', rough=0.6)
    for i, (top, val, base, ink2) in enumerate(CHIPS):
        x, y, w, h = A.region(f'chip{i}', 39, 39)
        chip_print(s, x, y, 39, top, val, base, ink2)
        x, y, w, h = A.region(f'chipe{i}', 60, 6)
        chip_edge(s, x, y, w, h, base, ink2)
    x, y, w, h = A.region('button', 60, 60)
    s.rect(x, y, w, h, 0xf2efe8, rough=0.2)
    s.ring(x + 30, y + 30, 26, 27.5, 0x161616, rough=0.2)
    s.text('DEALER', x + 30, y + 25.5, 10.5, 0x161616, face='Black', align='CENTER', rough=0.2)
    A.fill('btn_edge', 30, 6, 0xe6e2d8, 0.2)
    A.render()
    return A


def book(name, w, h, d, atlas, key, mat_print, mat_body, color, matrix):
    """A paperback standing on its tail, its spine (printed) toward -Y."""
    o = parts.rbox(name, (0, 0, 0), (w, d, h), 0.0012, 1)
    pat_faces(o, box_pat(atlas, (w, d, h), {'-y': key}, default='pages'))
    me = o.data
    me.materials.append(mat_print.m)
    me.materials.append(mat_body.m)
    ns = np.empty(len(me.polygons) * 3)
    me.polygons.foreach_get('normal', ns)
    ns = ns.reshape(-1, 3)
    me.polygons.foreach_set('material_index', np.where(ns[:, 1] < -0.6, 0, 1).astype(np.int32))
    # cover color on the covers, page edges elsewhere (the body shader reads dr_col)
    a = me.attributes.new('dr_col', 'FLOAT_COLOR', 'FACE')
    pages = (np.abs(ns[:, 2]) > 0.6) | (ns[:, 1] > 0.6)
    cols = np.where(pages[:, None], np.array(core.hex_linear(0xd8cfb8)), np.array(core.hex_linear(color)))
    a.data.foreach_set('color', cols.astype(np.float32).ravel())
    me.transform(matrix)
    return o


def frame_obj(name, outer, inner, depth, atlas, key, mat_print, mat_frame, k, matrix, lean=12.0, easel=True):
    """A picture frame standing on its easel: outer / inner (photo) sizes (w, h), its front toward -Y, leaning back."""
    ow, oh = outer
    iw, ih = inner
    parts_ = []
    fr = parts.outline_prism('frame', core.rounded_rect(-ow / 2, -oh / 2, ow / 2, oh / 2, 0.002, steps=2),
                             [list(reversed(core.rounded_rect(-iw / 2, -ih / 2, iw / 2, ih / 2, 0.0005, steps=1)))], 0.0, depth)
    core.assign(fr, mat_frame)
    kind(fr, k)
    parts_.append(fr)
    ph = print_panel('photo', core.rounded_rect(-iw / 2 - 0.002, -ih / 2 - 0.002, iw / 2 + 0.002, ih / 2 + 0.002, 0.0, steps=1),
                     depth * 0.35, depth * 0.35 + 0.001, atlas, key, side=key)
    core.assign(ph, mat_print)
    kind(ph, 0)
    parts_.append(ph)
    back = parts.rbox('frame_back', (0, 0, depth * 0.2), (ow - 0.004, oh - 0.004, depth * 0.4), 0.001)
    core.assign(back, mat_frame)
    kind(back, 5)
    parts_.append(back)
    # into place: face +Z -> -Y, standing, leaning back
    M = matrix @ Matrix.Rotation(math.radians(-lean), 4, 'X') @ Matrix.Translation((0, 0, oh / 2)) @ Matrix.Rotation(math.radians(90), 4, 'X')
    for p_ in parts_:
        p_.data.transform(M)
    if easel:
        # the strut: hinged on the back 60% of the way up (just behind it), down and back to the shelf at 30 degrees
        # (it used to start in front of the leaning back and came out through the photograph)
        s_, ln = 0.6 * oh, math.radians(lean)
        hy, hz = s_ * math.sin(ln) + 0.0032, s_ * math.cos(ln)
        L = hz / math.cos(math.radians(30))
        st = parts.rbox('easel', (0, 0, 0), (0.025, 0.004, L), 0.001)
        st.data.transform(matrix @ Matrix.Translation((0, hy + 0.5 * hz * math.tan(math.radians(30)), hz / 2)) @ Matrix.Rotation(math.radians(30), 4, 'X'))
        core.assign(st, mat_frame)
        kind(st, 5)
        parts_.append(st)
    return parts_


def build_shelf():
    """SM_DR_Shelf: a stained pine bookcase, 0.80 x 0.30 x 1.80 m (shelves at S_LEVELS), its middle shelf sagging.
    Photo albums and Dee's wooden chip case at the bottom; a row of paperbacks; her photographs (the young dealer at
    the Riverside, a beach, a school portrait, a party, a wedding); the Riverside chip frame; a dealer button on a
    stand; the BEST DEALER 1998 plaque; three stacks of clay chips; a trailing pothos on top, hanging down the front."""
    rng = random.Random(21)
    books = []
    rows = {1: 24, 4: 13}
    plan = []
    for lvl, n in rows.items():
        for i in range(n):
            t = TITLES[(len(books) * 7) % len(TITLES)]
            w = rng.uniform(0.017, 0.033)
            h = rng.uniform(0.172, 0.208)
            books.append((t[0], t[1], w, h, rng.choice(SPINE_COLORS)))
            plan.append(lvl)
    # four lying on shelf 3
    for i in range(4):
        t = TITLES[(i * 5 + 3) % len(TITLES)]
        books.append((t[0], t[1], rng.uniform(0.02, 0.03), rng.uniform(0.175, 0.2), rng.choice(SPINE_COLORS)))
        plan.append(3)
    A = shelf_atlas(books)
    wh = wood('drs_wood', 0x8a5a30, 0x4a2c14, axis='X', ring=170.0, rough=0.4, wear=0.7, grime=0.4, pores=0.6, center=(0.03, 0.4))
    wv = wood('drs_woodv', 0x8a5a30, 0x4a2c14, axis='Z', ring=170.0, rough=0.4, wear=0.7, grime=0.4, grime_z=(0.0, 0.1), pores=0.6, center=(0.02, 0.4))
    pr = print_material('drs_print', A)
    bd = core.Mat('drs_body')
    tc, sep = object_coords(bd)
    o = tc.outputs['Object']
    k = attr(bd, 'dr_k')
    ca = bd.node('ShaderNodeAttribute')
    ca.attribute_type = 'GEOMETRY'
    ca.attribute_name = 'dr_col'
    wear_ = ramp(bd, noise(bd, o, scale=60.0, detail=3.0), 0.55, 0.75)
    cover = bd.mix(bd.math('MULTIPLY', wear_, 0.25), ca.outputs['Color'], H(0xd8d0bc))
    col = cover
    col = bd.mix(is_k(bd, k, 1), col, H(0x121212))
    col = bd.mix(is_k(bd, k, 2), col, H(0xb8913e))
    col = bd.mix(is_k(bd, k, 3), col, bd.mix(ramp(bd, noise(bd, o, scale=800.0, detail=2.0), 0.3, 0.7), H(0x4a0e14), H(0x62161c)))
    walnut = bd.mix(ramp(bd, noise(bd, scaled(bd, sep, 40.0, 400.0, 40.0), detail=3.0), 0.3, 0.7), H(0x3a2214), H(0x5a3820))
    col = bd.mix(is_k(bd, k, 4), col, walnut)
    col = bd.mix(is_k(bd, k, 5), col, H(0x3a2c20))
    col = bd.mix(is_k(bd, k, 6), col, H(0x6a4a2c))
    bd.set('Base Color', col)
    bd.set('Metallic', mix_float(bd, is_k(bd, k, 2), 0.0, 1.0))
    rough = mix_float(bd, is_k(bd, k, 1), 0.55, 0.25)
    rough = mix_float(bd, is_k(bd, k, 2), rough, 0.3)
    rough = mix_float(bd, is_k(bd, k, 3), rough, 0.95)
    rough = mix_float(bd, is_k(bd, k, 4), rough, 0.3)
    bd.set('Roughness', rough)
    set_bump(bd, noise(bd, o, scale=900.0, detail=2.0), 0.08, 0.0002)
    mt = brass('drs_metal', 0x9a7a3a, 0.5, 0.35)
    pm = plant_material('drs_plant', 0xa85a34, 'terracotta')
    out = []

    def add(obj, m, kk=0):
        if m is not None:
            core.assign(obj, m)
        kind(obj, kk)
        out.append(obj)
        return obj

    hw, hd = S_W / 2, S_D / 2
    # the carcass: sides (with a toe-kick cutout), top, shelves, kick, back
    for s in (-1, 1):
        add(box(s * hw - (S_T if s > 0 else 0), s * hw + (0 if s > 0 else S_T), -hd, hd, 0.0, S_H - S_T, 0.002), wv)
    add(box(-hw - 0.012, hw + 0.012, -hd - 0.014, hd, S_H - S_T, S_H, 0.005), wh)
    for i, z in enumerate(S_LEVELS):
        sh = add(box(-hw + S_T, hw - S_T, -hd + 0.004, hd - 0.006, z, z + S_T, 0.002), wh)
        if i == 2:
            P = verts_np(sh)
            P[:, 2] -= 0.006 * np.clip(1 - (P[:, 0] / (hw - S_T)) ** 2, 0, 1)
            set_verts(sh, P)
    add(box(-hw + S_T, hw - S_T, -hd + 0.01, -hd + 0.03, 0.0, 0.07, 0.002), wh)
    add(box(-hw + 0.004, hw - 0.004, hd - 0.006, hd - 0.002, 0.0, S_H - S_T, 0.0), wv, 0)
    # bottom shelf: albums standing, the chip case
    z0 = S_LEVELS[0] + S_T
    for i in range(3):
        add(print_box('album', (0.058, 0.27, 0.3), A, {'-y': f'album{i}'}, default='pages', r=0.004,
                      matrix=mat4((-hw + S_T + 0.04 + i * 0.061, -hd + 0.15, z0 + 0.15))), pr)
    case = add(rbox('chipcase', (0.13, -0.01, z0 + 0.04), (0.33, 0.22, 0.08), 0.006), bd, 4)
    add(rbox('chipcase_latch', (0.13, -0.122, z0 + 0.055), (0.03, 0.006, 0.02), 0.002), mt)
    for hx in (0.04, 0.22):
        add(rbox('hinge', (hx, 0.1, z0 + 0.06), (0.03, 0.004, 0.02), 0.001), mt)
    # paperbacks
    bi = 0
    for lvl in (1, 4):
        x = -hw + S_T + 0.006
        zb = S_LEVELS[lvl] + S_T
        n = rows[lvl]
        for j in range(n):
            title, author, w, h, color = books[bi]
            d = rng.uniform(0.105, 0.125)
            lean = 0.0
            if j == n - 1:
                lean = -9.0
            M = mat4((x + w / 2 + (0.016 if lean else 0.0), -hd + 0.03 + d / 2, zb + h / 2 + (-0.002 if lean else 0.0)), ry=lean)
            out.append(book('book', w, h, d, A, f'spine{bi}', pr, bd, color, M))
            kind(out[-1], 0)
            x += w + 0.0008
            bi += 1
        if lvl == 1:
            # a bookend, then a few lying on their sides
            add(rbox('bookend', (x + 0.045, -hd + 0.09, zb + 0.08), (0.003, 0.12, 0.16), 0.001), mt)
            add(rbox('bookend_foot', (x + 0.08, -hd + 0.09, zb + 0.0015), (0.07, 0.12, 0.003), 0.001), mt)
    # shelf 2: photographs, the chip frame at the back, the dealer button on its stand
    z2 = S_LEVELS[2] + S_T - 0.003
    for key, outer, inner, cx, cy, rz, k_, lean in (('ph_dealer', (0.125, 0.175), (0.102, 0.152), -0.26, -0.06, 12.0, 2, 12.0),
                                                     ('ph_beach', (0.175, 0.125), (0.152, 0.102), 0.2, -0.07, -10.0, 1, 12.0),
                                                     ('ph_school', (0.11, 0.15), (0.089, 0.127), -0.1, -0.09, 4.0, 1, 12.0)):
        out += frame_obj('frame', outer, inner, 0.014, A, key, pr, bd, k_, mat4((cx, cy, z2), rz=rz), lean)
    # the shadow box of chips, leaning on the back
    fw, fh, fd = 0.30, 0.22, 0.035
    Mf = mat4((0.02, hd - 0.075, z2), rx=-8.0)
    box_parts = []
    fr = parts.outline_prism('chipframe', core.rounded_rect(-fw / 2, -fh / 2, fw / 2, fh / 2, 0.002, steps=2),
                             [list(reversed(core.rounded_rect(-fw / 2 + 0.022, -fh / 2 + 0.022, fw / 2 - 0.022, fh / 2 - 0.022, 0.0, steps=1)))], 0.0, fd)
    core.assign(fr, bd)
    kind(fr, 1)
    felt = parts.rbox('felt', (0, 0, 0.004), (fw - 0.03, fh - 0.03, 0.008), 0.0)
    core.assign(felt, bd)
    kind(felt, 3)
    box_parts += [fr, felt]
    for i in range(8):
        cx = -0.105 + (i % 4) * 0.07
        cy = 0.035 - (i // 4) * 0.055
        ch = chip_obj('chip', A, f'chip{i}', f'chipe{i}', matrix=Matrix.Translation((cx, cy, 0.008 + 0.0017)))
        core.assign(ch, pr)
        kind(ch, 0)
        box_parts.append(ch)
    lab = print_box('frame_label', (0.11, 0.016, 0.0012), A, {'+z': 'frame_label'}, default='frame_label', matrix=Matrix.Translation((0.0, -0.075, 0.0086)))
    core.assign(lab, pr)
    kind(lab, 0)
    box_parts.append(lab)
    M = Mf @ Matrix.Translation((0, 0, fh / 2)) @ Matrix.Rotation(math.radians(90), 4, 'X') @ Matrix.Translation((0, 0, -fd))
    for p_ in box_parts:
        p_.data.transform(M)
        out.append(p_)
    # the dealer button on a little walnut stand
    bc = Vector((0.0, -0.085, z2))
    add(rbox('btn_stand', bc + Vector((0, 0.0, 0.009)), (0.07, 0.035, 0.018), 0.004), bd, 4)
    btn = parts.disc('button', (0, 0, 0), (0, 0, 1), 0.03, 0.012, 48)

    def bfn(P, N):
        side = np.abs(N[:, 2]) < 0.5
        uv = A.uv('button', P[:, 0] / 0.06 + 0.5, P[:, 1] / 0.06 + 0.5)
        uv[side] = A.uv('btn_edge', np.full(side.sum(), 0.5), np.full(side.sum(), 0.5))
        return uv
    pat_faces(btn, bfn)
    btn.data.transform(Matrix.Translation(bc + Vector((0, 0.004, 0.012 + 0.03))) @ Matrix.Rotation(math.radians(80), 4, 'X'))
    add(btn, pr)
    # shelf 3: the plaque leaning on the back, a stack of paperbacks, three stacks of chips
    z3 = S_LEVELS[3] + S_T
    Mp = mat4((-0.17, hd - 0.04, z3), rz=4.0, rx=-12.0)
    pl_ = parts.rbox('plaque', (0, 0, 0.125), (0.2, 0.018, 0.25), 0.006, 3)
    pl_.data.transform(Mp)
    add(pl_, bd, 4)
    plate = print_box('plate', (0.15, 0.0015, 0.105), A, {'-y': 'plate'}, default='plate', r=0.0004,
                      matrix=Mp @ Matrix.Translation((0, -0.0098, 0.135)))
    add(plate, pr)
    for sx in (-0.06, 0.06):
        for sz in (0.088, 0.182):
            add(parts.disc('screw', (sx, -0.0108, sz), (0, 1, 0), 0.0028, 0.0012, 10), mt)
            out[-1].data.transform(Mp)
    zz = z3
    for i in range(4):
        title, author, w, h, color = books[bi]
        d = 0.115
        # lying face up, their tops to the left, so the spines read left to right
        M = mat4((0.17 + rng.uniform(-0.006, 0.006), -0.02, zz + w / 2), rz=rng.uniform(-5, 5)) @ Matrix.Rotation(math.radians(-90), 4, 'Y')
        out.append(book('book', w, h, d, A, f'spine{bi}', pr, bd, color, M))
        kind(out[-1], 0)
        zz += w
        bi += 1
    for sx, sy, n, ci in ((0.04, -0.1, 7, 0), (0.085, -0.075, 4, 1), (0.03, -0.045, 10, 2)):
        for j in range(n):
            ch = chip_obj('chip', A, f'chip{ci}', f'chipe{ci}', matrix=mat4((sx + rng.uniform(-0.0012, 0.0012), sy + rng.uniform(-0.0012, 0.0012), z3 + 0.00165 + j * 0.0034), rz=rng.uniform(0, 360)))
            add(ch, pr)
    # shelf 4 (top shelf inside): two more photographs at the right
    z4 = S_LEVELS[4] + S_T
    for key, outer, inner, cx, cy, rz, k_ in (('ph_party', (0.175, 0.125), (0.152, 0.102), 0.2, -0.06, -6.0, 2), ('ph_wedding', (0.125, 0.15), (0.102, 0.127), 0.31, -0.02, -18.0, 1)):
        out += frame_obj('frame', outer, inner, 0.016, A, key, pr, bd, k_, mat4((cx, cy, z4), rz=rz), 11.0)
    # the pothos on top, trailing down the front
    zt = S_H
    pot_c = Vector((0.2, -0.02, zt))
    pot = lathe('pot', [(0.0, 0.0), (0.05, 0.0), (0.052, 0.006), (0.062, 0.09), (0.068, 0.096), (0.068, 0.108), (0.061, 0.108), (0.059, 0.095),
                        (0.05, 0.012), (0.0, 0.012)], 48, Matrix.Translation(pot_c))
    add(pot, pm, 3)
    add(lathe('saucer', [(0.0, 0.0), (0.066, 0.0), (0.074, 0.012), (0.071, 0.013), (0.062, 0.004), (0.0, 0.004)], 48, Matrix.Translation(pot_c)), pm, 4)
    add(lathe('soil', [(0.0, 0.094), (0.04, 0.093), (0.06, 0.09), (0.059, 0.08), (0.0, 0.08)], 32, Matrix.Translation(pot_c)), pm, 2)
    vines, leaves, pets = pothos(rng, pot_c, [(-20, 'up', 0.14), (40, 'up', 0.1), (170, 'up', 0.12), (-10, 'trail', 0.5), (25, 'trail', 0.75),
                                              (-45, 'trail', 0.38), (70, 'side', 0.2), (-80, 'side', 0.3), (5, 'trail', 0.95)],
                                 0.092, 0.06, drop_edge=(-hd - 0.014, 0.0))
    # trails: hang them down the front by their own reach
    for v in vines:
        add(v, pm, 1)
    for p_ in pets:
        add(p_, pm, 1)
    for lf in leaves:
        add(lf, pm, 0)
    sh_obj = finish(out, 'SM_DR_Shelf', [wh, wv, pr, bd, mt, pm], angle=40.0)
    return [sh_obj]


# ================================================================== SM_DR_Plant

def build_plant():
    """SM_DR_Plant: a snake plant (about 0.85 m to its tallest leaf) in a cobalt drip-glaze pot (0.26 across) on a
    saucer: fifteen banded, yellow-edged leaves, one creased and leaning, one with a dry brown tip; dusty."""
    rng = random.Random(33)
    m = core.Mat('drp_leaf')
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    pu = m.node('ShaderNodeSeparateXYZ')
    m.link(uv_coords(m), pu.inputs['Vector'])
    U, V = pu.outputs['X'], pu.outputs['Y']
    k = attr(m, 'dr_k')
    wav = m.math('ADD', m.math('MULTIPLY', V, 60.0), m.math('MULTIPLY', m.math('ABSOLUTE', m.math('SUBTRACT', U, 0.5)), 6.0))
    bands = ramp(m, m.math('SINE', m.math('ADD', wav, m.math('MULTIPLY', noise(m, o, scale=30.0, detail=3.0), 4.0))), 0.35, 0.8)
    dark = m.mix(ramp(m, noise(m, o, scale=20.0, detail=3.0), 0.3, 0.7), H(0x1c3a1a), H(0x284a22))
    col = m.mix(m.math('MULTIPLY', bands, 0.65), dark, H(0x6e8a5a))
    margin = m.math('MAXIMUM', smooth_less(m, U, 0.07, 0.015), smooth_less(m, m.math('SUBTRACT', 1.0, U), 0.07, 0.015))
    col = m.mix(margin, col, H(0xc8b64a))
    tip = m.math('MULTIPLY', ramp(m, V, 0.86, 0.93), is_k(m, k, 1))
    col = m.mix(tip, col, H(0x7a5a2a))
    dust = m.math('MULTIPLY', ramp(m, noise(m, o, scale=50.0, detail=3.0), 0.45, 0.85), 0.3)
    col = m.mix(dust, col, H(0x8a8a78))
    m.set('Base Color', col)
    m.set('Roughness', m.math('ADD', 0.32, m.math('MULTIPLY', dust, 0.8)))
    m.set('Subsurface Weight', 0.08)
    set_bump(m, m.math('ADD', m.math('MULTIPLY', bands, 0.2), m.math('MULTIPLY', tip, 0.5)), 0.2, 0.001)
    pm = plant_material('drp_pot', 0x1e3a8a, 'glaze')
    out = []
    # the pot, its rim, the saucer, the soil
    add_ = lambda obj, mat, kk: (core.assign(obj, mat), kind(obj, kk), out.append(obj))
    add_(lathe('pot', [(0.0, 0.012), (0.085, 0.012), (0.09, 0.02), (0.11, 0.17), (0.128, 0.2), (0.13, 0.215), (0.12, 0.218), (0.112, 0.205),
                       (0.1, 0.03), (0.0, 0.03)], 64), pm, 3)
    add_(lathe('saucer', [(0.0, 0.0), (0.105, 0.0), (0.118, 0.016), (0.114, 0.017), (0.1, 0.004), (0.0, 0.004)], 64), pm, 4)
    add_(lathe('soil', [(0.0, 0.18), (0.08, 0.178), (0.11, 0.175), (0.108, 0.16), (0.0, 0.16)], 40), pm, 2)
    for i in range(18):
        ang = rng.uniform(0, 2 * math.pi)
        rr = rng.uniform(0.0, 0.07)
        base = Vector((rr * math.cos(ang), rr * math.sin(ang), 0.165))
        L = rng.uniform(0.42, 0.82) * (0.8 if rr > 0.05 else 1.0)
        wmax = rng.uniform(0.045, 0.065)
        lean = rng.uniform(0.0, 0.18) + rr * 1.2
        outd = Vector((math.cos(ang), math.sin(ang), 0.0)) if rr > 0.005 else Vector((1, 0, 0))
        twist = rng.uniform(-1.2, 1.2)
        kink = i == 3
        dry = i in (5, 11)
        nv, nu = 22, 6
        rings = []
        uvs = []
        for j in range(nv + 1):
            t = j / nv
            w = wmax * (0.35 + 1.9 * t * (1 - t) ** 0.7 if t < 0.92 else 0.0)
            w = max(w, 0.0) * (1.0 if t < 0.9 else (1 - t) / 0.1)
            # the axis: up, leaning out, with a kink for one leaf
            lean_t = lean * t * t + (0.35 * max(t - 0.55, 0.0) if kink else 0.0)
            c = base + outd * (math.sin(lean_t) * L * t * 0.6) + Vector((0, 0, L * t * math.cos(min(lean_t, 1.2))))
            if kink and t > 0.55:
                c += outd * (t - 0.55) * L * 0.45 - Vector((0, 0, (t - 0.55) * L * 0.15))
            th = twist * t + ang
            side = Vector((-math.sin(th), math.cos(th), 0.0))
            nrm = Vector((math.cos(th), math.sin(th), 0.0))
            ring = []
            for q in range(nu):
                s_ = q / (nu - 1) - 0.5
                ring.append(c + side * s_ * w + nrm * (-(s_ * s_) * w * 0.6 + 0.0015))
                uvs.append((q / (nu - 1), t))
            for q in range(nu - 1, -1, -1):
                s_ = q / (nu - 1) - 0.5
                ring.append(c + side * s_ * w * 0.96 + nrm * (-(s_ * s_) * w * 0.6 - 0.0015))
                uvs.append((q / (nu - 1), t))
            rings.append(ring)
        bm = bmesh.new()
        vs = [[bm.verts.new(tuple(p)) for p in r] for r in rings]
        m2 = len(rings[0])
        pat = {}
        for j, r in enumerate(vs):
            for q, v in enumerate(r):
                pat[v] = uvs[j * m2 + q]
        for a_, b_ in zip(vs, vs[1:]):
            for q in range(m2):
                try:
                    bm.faces.new((a_[q], a_[(q + 1) % m2], b_[(q + 1) % m2], b_[q]))
                except ValueError:
                    pass
        bm.faces.new(list(reversed(vs[0])))
        bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-6)
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        uvl = bm.loops.layers.uv.new('UVMap')
        pl = bm.loops.layers.uv.new('pat')
        for f in bm.faces:
            for l in f.loops:
                p = pat.get(l.vert, (0.5, 1.0))
                l[pl].uv = p
                l[uvl].uv = p
        leaf = core.mesh_object('sword', bm)
        leaf.data.uv_layers.active = leaf.data.uv_layers['UVMap']
        leaf.data.shade_smooth()
        add_(leaf, m, 1 if dry else 0)
    return [finish(out, 'SM_DR_Plant', [m, pm], angle=60.0, weighted=False)]


# ================================================================== paint (shared)

def old_paint(name, color, under1=0xe6dfcc, under2=0x5e745a, base=0x7a5a3c, chips=1.0, crackle=0.0, grime=None, gloss=0.38, colors=None):
    """Woodwork painted over many times: semi-gloss, brush marks and drips frozen in it, chipped on the edges through
    an older cream and an older green to the wood, grime in the corners and where hands go (grime(m, sep) -> mask).
    colors: optional {dr_k: hex} top coats for parts of one slot (a door and its trim)."""
    m = core.Mat(name)
    tc, sep = object_coords(m)
    o = tc.outputs['Object']
    top = H(color)
    if colors:
        k = attr(m, 'dr_k')
        for kk, hx in colors.items():
            top = m.mix(is_k(m, k, kk), top, H(hx))
    edge = convex_edges(m, tc, distance=0.0025, samples=12, breakup_scale=50.0, gain=5.0)
    blot = noise(m, o, scale=30.0, detail=5.0, roughness=0.65)
    c1 = m.math('MULTIPLY', m.math('MAXIMUM', ramp(m, m.math('MULTIPLY', edge, ramp(m, blot, 0.3, 0.6)), 0.3, 0.45), ramp(m, blot, 0.74, 0.77)), chips)
    c2 = m.math('MULTIPLY', c1, ramp(m, noise(m, o, scale=45.0, detail=4.0), 0.48, 0.56))
    c3 = m.math('MULTIPLY', c2, ramp(m, noise(m, o, scale=70.0, detail=4.0), 0.55, 0.62))
    mott = noise(m, o, scale=3.0, detail=3.0)
    col = m.mix(m.math('MULTIPLY', mott, 0.12), top, H(0x8a8070))
    col = m.mix(c1, col, H(under1))
    col = m.mix(c2, col, H(under2))
    col = m.mix(c3, col, H(base))
    rough = m.math('ADD', gloss, m.math('MULTIPLY', mott, 0.1))
    rough = mix_float(m, c1, rough, 0.5)
    rough = mix_float(m, c3, rough, 0.8)
    brush = noise(m, scaled(m, sep, 300.0, 300.0, 18.0), detail=2.0)
    drip = ramp(m, noise(m, scaled(m, sep, 160.0, 160.0, 5.0), detail=2.0), 0.68, 0.76)
    height = m.math('ADD', m.math('MULTIPLY', brush, 0.25), m.math('MULTIPLY', drip, 0.7))
    height = m.math('SUBTRACT', height, m.math('MULTIPLY', c1, 0.6))
    if crackle > 0:
        cr = m.node('ShaderNodeTexVoronoi', Scale=170.0)
        cr.feature = 'DISTANCE_TO_EDGE'
        m.link(o, cr.inputs['Vector'])
        crack = m.math('MULTIPLY', smooth_less(m, cr.outputs['Distance'], 0.03, 0.012), crackle)
        col = m.mix(m.math('MULTIPLY', crack, 0.7), col, H(0x3a3026))
        height = m.math('SUBTRACT', height, m.math('MULTIPLY', crack, 0.6))
    cav = cavity(m, 0.02, 1.6)
    col = m.mix(m.math('MULTIPLY', cav, 0.55), col, H(0x4a4034))
    rough = mix_float(m, cav, rough, 0.7)
    if grime is not None:
        g = m.math('MULTIPLY', grime(m, sep), ramp(m, noise(m, o, scale=10.0, detail=4.0), 0.25, 0.7))
        col = m.mix(m.math('MULTIPLY', g, 0.6), col, H(0x5a4a38))
        rough = mix_float(m, g, rough, 0.3)
    m.set('Base Color', col)
    m.set('Roughness', m.math('MINIMUM', rough, 0.92))
    set_bump(m, height, 0.18, 0.0008)
    return m


def mug(name, atlas, region, r=0.041, h=0.095, matrix=None):
    """A stoneware mug: printed round its outside (region), plain inside; a C handle. Returns [body, handle]."""
    body = lathe('mug', [(0.0, 0.0), (r - 0.006, 0.0), (r - 0.002, 0.003), (r, 0.01), (r, h - 0.002), (r - 0.001, h), (r - 0.004, h),
                         (r - 0.0045, h - 0.004), (r - 0.0045, 0.012), (0.0, 0.009)], 40)

    def fn(P, N):
        rad = np.hypot(P[:, 0], P[:, 1])
        ang = (np.arctan2(P[:, 1], P[:, 0]) / (2 * np.pi) + 0.25) % 1.0
        outside = (rad > r - 0.0015) & (np.abs(N[:, 2]) < 0.6)
        uv = atlas.uv(region, np.full(len(P), 0.02), np.full(len(P), 0.98))
        uv[outside] = atlas.uv(region, ang[outside], np.clip(P[outside, 2] / h, 0, 1))
        return uv
    pat_faces(body, fn)
    hp = [(r - 0.003, 0.0, h * 0.78), (r + 0.022, 0.0, h * 0.8), (r + 0.03, 0.0, h * 0.55), (r + 0.022, 0.0, h * 0.3), (r - 0.003, 0.0, h * 0.25)]
    handle = core.sweep('handle', [Vector(p) for p in core.catmull_rom(hp, 6)], [(0.0055 * math.cos(2 * math.pi * i / 10), 0.0035 * math.sin(2 * math.pi * i / 10)) for i in range(10)])
    pat_faces(handle, lambda P, N: atlas.uv(region, np.full(len(P), 0.02), np.full(len(P), 0.5)))
    if matrix is not None:
        body.data.transform(matrix)
        handle.data.transform(matrix)
    return [body, handle]


# ================================================================== SM_DR_Kitchenette

K_W, K_D, K_H = 1.20, 0.62, 0.91
K_BACK = K_D / 2          # the wall the counter stands against (and the calendar hangs on)
K_SINK = (-0.36, -0.01)   # the bowl's middle
K_HOT = (0.33, -0.12)     # the hot plate's middle


def kitchen_atlas():
    A = Atlas('drk_prints', 600.0, 640.0, 2048)
    s = A.s
    # the Wash & Fold calendar, October, Dee's game nights circled
    x, y, w, h = A.region('calendar', 300, 460)
    s.rect(x, y, w, h, 0xf1ece0, rough=0.7)
    s.rect(x, y + h - 42, w, 42, 0xb3202a, rough=0.6)
    s.text('WASH & FOLD', x + w / 2, y + h - 27, 21, 0xf6f0e2, face='Black', align='CENTER', tracking=1.1, rough=0.6)
    s.text('5TH STREET · NORTHSIDE · OPEN 7 AM TO 11 PM', x + w / 2, y + h - 37, 6.4, 0xf6d8c8, face='Bold', align='CENTER', rough=0.6)
    A.paste((x + 8, y + 232, w - 16, 180), photo('lighthouse', 420, 252, seed=7), rough=0.4)
    s.text('OCTOBER', x + 12, y + 208, 18, 0x1a1a1a, face='Black', rough=0.7)
    for i, d in enumerate('SMTWTFS'):
        s.text(d, x + 12 + i * 39.4 + 19.7, y + 192, 7, 0xb3202a if i in (0, 6) else 0x1a1a1a, face='Bold', align='CENTER', rough=0.7)
    gx0, gy0, cw, chh = x + 12, y + 18, 39.4, 33.2
    for r_ in range(6):
        s.rect(gx0, gy0 + r_ * chh, 7 * cw, 0.5, 0x8a8478, rough=0.7)
    for c_ in range(8):
        s.rect(gx0 + c_ * cw, gy0, 0.5, 5 * chh, 0x8a8478, rough=0.7)
    for day in range(1, 32):
        idx = day + 3  # October 1st is a Thursday
        r_, c_ = idx // 7, idx % 7
        cx, cy = gx0 + c_ * cw, gy0 + (4 - r_) * chh
        if r_ > 4:
            continue
        s.text(str(day), cx + cw - 4, cy + chh - 9, 7, 0x1a1a1a, face='Bold', align='RIGHT', rough=0.7)
        if c_ in (2, 4, 6):
            s.ring(cx + cw / 2 - 2, cy + chh / 2 - 4, 9.5, 10.6, 0xc0141c, rough=0.7)
            s.text('9 PM', cx + cw / 2 - 2, cy + chh / 2 - 7, 5.5, 0xc0141c, face='Medium', align='CENTER', rough=0.7)
        if day < 5:
            s.rect(cx + 3, cy + 3, cw - 6, 0.9, 0x22222a, rough=0.7, rot=math.atan2(chh - 6, cw - 6))
            s.rect(cx + 3, cy + chh - 3, cw - 6, 0.9, 0x22222a, rough=0.7, rot=-math.atan2(chh - 6, cw - 6))
    for day, note, colr in ((14, 'DR 10:30', 0x1a3a8a), (22, 'MEI BDAY', 0x1a3a8a), (30, 'RENT', 0x1a1a1a)):
        idx = day + 3
        r_, c_ = idx // 7, idx % 7
        cx, cy = gx0 + c_ * cw, gy0 + (4 - r_) * chh
        s.text(note, cx + 3, cy + 4, 4.6, colr, face='Medium', rough=0.7)
    s.text('FLUFF & FOLD 99¢ A POUND · DROP-OFF SERVICE', x + w / 2, y + 6, 6, 0xb3202a, face='Bold', align='CENTER', rough=0.7)
    # mugs, the tea tin, the tag, the hot plate's panel
    x, y, w, h = A.region('mug_riverside', 258, 95)
    s.rect(x, y, w, h, 0x1d4a2e, rough=0.15)
    s.rect(x, y + 78, w, 2.5, 0xc8a050, rough=0.2, metal=0.8)
    s.rect(x, y + 12, w, 2.5, 0xc8a050, rough=0.2, metal=0.8)
    s.text('RIVERSIDE', x + w * 0.5, y + 50, 13, 0xf0e6c8, face='Black', align='CENTER', tracking=1.2, rough=0.15)
    s.text('CARD ROOM', x + w * 0.5, y + 37, 7.5, 0xc8a050, face='Bold', align='CENTER', tracking=1.4, rough=0.15)
    for k_, st in enumerate('SHDC'):
        suit(s, st, x + w * 0.5 - 24 + k_ * 16, y + 25, 7, 0xf0e6c8)
    x, y, w, h = A.region('mug_brown', 30, 30)
    s.rect(x, y, w, h, 0x6a4024, rough=0.25)
    for k_ in range(40):
        s.circle(x + (k_ * 7.3) % 30, y + (k_ * 11.7) % 30, 0.5, 0x2a1a10, rough=0.25, n=6)
    x, y, w, h = A.region('mug_diner', 258, 95)
    s.rect(x, y, w, h, 0xeeeae0, rough=0.12)
    s.rect(x, y + 74, w, 4, 0x2a4a8a, rough=0.12)
    s.rect(x, y + 68, w, 1.5, 0x2a4a8a, rough=0.12)
    x, y, w, h = A.region('tea', 315, 130)
    s.rect(x, y, w, h, 0x8e1c1c, rough=0.3, metal=0.4)
    s.rect(x, y + 98, w, 3, 0xd8b050, rough=0.25, metal=1.0)
    s.rect(x, y + 25, w, 3, 0xd8b050, rough=0.25, metal=1.0)
    for rep in (0.25, 0.75):
        s.text('TEA', x + w * rep, y + 52, 26, 0xe8c870, face='Black', align='CENTER', tracking=1.3, rough=0.3, metal=0.8)
    x, y, w, h = A.region('tag', 22, 28)
    s.rect(x, y, w, h, 0xf2ede0, rough=0.7)
    s.rect(x + 2, y + 2, w - 4, h - 4, 0xd8c070, rough=0.7)
    s.text('OOLONG', x + w / 2, y + 12, 3.6, 0x3a2a10, face='Bold', align='CENTER', rough=0.7)
    x, y, w, h = A.region('hp_front', 230, 35)
    s.rect(x, y, w, h, 0x1b1b1c, rough=0.4)
    for cx in (x + 60, x + 170):
        for k_, lab in enumerate(('OFF', 'LO', 'MED', 'HI')):
            a = math.radians(140 - k_ * 45)
            s.text(lab, cx + 13 * math.cos(a), y + 15 + 13 * math.sin(a) - 1.5, 3.4, 0xd8d8d0, face='Bold', align='CENTER', rough=0.4)
    s.text('THRIFT-KING', x + 115, y + 5, 4.4, 0xb8b8b0, face='Black', align='CENTER', tracking=1.2, rough=0.4)
    A.fill('white', 10, 10, 0xeeeae0, 0.3)
    A.fill('dark', 10, 10, 0x1a1a1a, 0.4)
    A.render()
    return A


def build_kitchenette():
    """SM_DR_Kitchenette: 1.20 x 0.62 x 0.91 m, its back against the wall (y = +0.31): a butter-yellow painted base
    cabinet (sink doors, a drawer with a brass bar and a door), a gold-fleck laminate top with an aluminum edge and
    splash, a stainless bar sink and a two-handle faucet, a two-ring hot plate with Dee's red enamel kettle, mugs (her
    Riverside mug with a teabag in it), a tea tin, a wire dish rack with plates, a bowl and a mug; a gingham towel over
    the drawer's bar; the Wash & Fold calendar on the wall (Tue/Thu/Sat circled: 9 PM)."""
    A = kitchen_atlas()

    def grime(m, sep):
        knobs = m.math('MULTIPLY', box_mask(m, sep, (None, None, 0.6), (None, -0.27, 0.87), 0.04), 1.0)
        kick = box_mask(m, sep, (None, None, None), (None, -0.2, 0.14), 0.03)
        return m.math('MAXIMUM', m.math('MULTIPLY', knobs, 0.6), kick)
    pt = old_paint('drk_paint', 0xd6c48a, grime=grime, gloss=0.4)

    def lam_top(m_unused=None):
        m = core.Mat('drk_counter')
        tc, sep = object_coords(m)
        o = tc.outputs['Object']
        k = attr(m, 'dr_k')
        fl = m.node('ShaderNodeTexVoronoi', Scale=380.0)
        m.link(o, fl.inputs['Vector'])
        fleck = m.math('MULTIPLY', smooth_less(m, fl.outputs['Distance'], 0.1, 0.03), ramp(m, noise(m, o, scale=200.0, detail=1.0), 0.45, 0.6))
        boom = ramp(m, noise(m, scaled(m, sep, 9.0, 14.0, 9.0), detail=2.0, distortion=1.5), 0.62, 0.66)
        col = m.mix(m.math('MULTIPLY', boom, 0.4), H(0xe2d6b4), H(0xc8a868))
        col = m.mix(fleck, col, H(0xa8782a))
        x, y = sep.outputs['X'], sep.outputs['Y']
        # wear: the front edge, a scorch ring by the hot plate, knife scars, a water stain round the sink
        burn_d = dist3(m, sep, (0.03, -0.02, K_H))
        burn = m.math('MULTIPLY', smooth_less(m, m.math('ABSOLUTE', m.math('SUBTRACT', burn_d, 0.07)), 0.012, 0.006), ramp(m, noise(m, o, scale=40.0, detail=3.0), 0.3, 0.6))
        col = m.mix(m.math('MULTIPLY', burn, 0.8), col, H(0x5a3a1a))
        stain = m.math('MULTIPLY', smooth_less(m, dist3(m, sep, (K_SINK[0], K_SINK[1], K_H)), 0.3, 0.06), ramp(m, noise(m, o, scale=12.0, detail=4.0), 0.45, 0.7))
        col = m.mix(m.math('MULTIPLY', stain, 0.35), col, H(0xa89878))
        front = m.math('MULTIPLY', smooth_less(m, m.math('ADD', y, K_D / 2), 0.06, 0.04), ramp(m, noise(m, o, scale=20.0, detail=3.0), 0.3, 0.7))
        col = m.mix(m.math('MULTIPLY', front, 0.3), col, H(0xb8aa88))
        rough = m.math('ADD', 0.32, m.math('ADD', m.math('MULTIPLY', front, 0.2), m.math('MULTIPLY', stain, 0.15)))
        alu = m.mix(ramp(m, noise(m, o, scale=40.0, detail=3.0), 0.4, 0.7), H(0xb8b8b4), H(0x8a8a84))
        col = m.mix(is_k(m, k, 1), col, alu)
        m.set('Base Color', col)
        m.set('Metallic', mix_float(m, is_k(m, k, 1), 0.0, 1.0))
        m.set('Roughness', mix_float(m, is_k(m, k, 1), rough, 0.3))
        set_bump(m, m.math('SUBTRACT', m.math('MULTIPLY', fleck, 0.3), m.math('MULTIPLY', burn, 0.5)), 0.08, 0.0003)
        return m
    ct = lam_top()
    stl = street.bare_metal('drk_steel', 0xa4a6a8, rough=0.28, grime=0.5)
    en = core.Mat('drk_enamel')
    tc, sep = object_coords(en)
    o = tc.outputs['Object']
    k = attr(en, 'dr_k')
    edge = ramp(en, convex_edges(en, tc, distance=0.0025, samples=12, breakup_scale=60.0, gain=5.0), 0.3, 0.48)
    chip = en.math('MULTIPLY', edge, ramp(en, noise(en, o, scale=50.0, detail=4.0), 0.5, 0.6))
    white = en.mix(ramp(en, noise(en, o, scale=10.0, detail=3.0), 0.4, 0.8), H(0xe6e0d0), H(0xc8bea4))
    red = en.mix(ramp(en, noise(en, o, scale=10.0, detail=3.0), 0.4, 0.8), H(0xa82418), H(0x8a1c12))
    m_rough = mix_float(en, is_k(en, k, 1), 0.2, 0.3)
    col = en.mix(is_k(en, k, 1), white, red)
    col = en.mix(is_k(en, k, 2), col, H(0x111111))
    col = en.mix(is_k(en, k, 3), col, H(0xe8e4dc))
    col = en.mix(is_k(en, k, 4), col, H(0x3a0a08))
    col = en.mix(en.math('MULTIPLY', chip, en.math('ADD', is_k(en, k, 0), is_k(en, k, 1))), col, H(0x1a1816))
    low = en.math('MULTIPLY', en.math('SUBTRACT', 1.0, ramp(en, sep.outputs['Z'], 0.99, 1.03)), is_k(en, k, 1))
    col = en.mix(en.math('MULTIPLY', low, 0.5), col, H(0x2a1a14))
    en.set('Base Color', col)
    en.set('Roughness', mix_float(en, chip, mix_float(en, is_k(en, k, 2), m_rough, 0.35), 0.6))
    en.set('Coat Weight', 0.3)
    pr = print_material('drk_print', A)
    rk = core.Mat('drk_rack')
    tc2, sep2 = object_coords(rk)
    o2 = tc2.outputs['Object']
    k2 = attr(rk, 'dr_k')
    rust = ramp(rk, noise(rk, o2, scale=90.0, detail=4.0), 0.68, 0.74)
    coat = rk.mix(rust, H(0xe8e6e0), H(0x7a3a18))
    col = rk.mix(is_k(rk, k2, 1), coat, H(0xf0ece2))
    col = rk.mix(is_k(rk, k2, 2), col, H(0xb8bab8))
    col = rk.mix(is_k(rk, k2, 3), col, H(0xe2ded2))
    rk.set('Base Color', col)
    rk.set('Metallic', mix_float(rk, is_k(rk, k2, 2), 0.0, 1.0))
    rk.set('Roughness', mix_float(rk, is_k(rk, k2, 1), mix_float(rk, rust, 0.35, 0.8), 0.12))
    tw = core.Mat('drk_towel')
    pu = tw.node('ShaderNodeSeparateXYZ')
    tw.link(uv_coords(tw), pu.inputs['Vector'])
    cu = tw.math('GREATER_THAN', tw.math('FRACT', tw.math('DIVIDE', pu.outputs['X'], 0.024)), 0.5)
    cv = tw.math('GREATER_THAN', tw.math('FRACT', tw.math('DIVIDE', pu.outputs['Y'], 0.024)), 0.5)
    both = tw.math('MULTIPLY', cu, cv)
    one = tw.math('SUBTRACT', tw.math('ADD', cu, cv), tw.math('MULTIPLY', both, 2.0))
    tcol = tw.mix(one, H(0xeee6da), H(0xd88a86))
    tcol = tw.mix(both, tcol, H(0xb02a2a))
    tc3, sep3 = object_coords(tw)
    weave = noise(tw, tc3.outputs['Object'], scale=1500.0, detail=1.0)
    tw.set('Base Color', tw.mix(tw.math('MULTIPLY', ramp(tw, noise(tw, tc3.outputs['Object'], scale=20.0, detail=3.0), 0.5, 0.8), 0.3), tcol, H(0x8a7a62)))
    tw.set('Roughness', 0.92)
    tw.set('Sheen Weight', 0.5)
    set_bump(tw, weave, 0.4, 0.0005)
    out = []

    def add(obj, mat, kk=0):
        core.assign(obj, mat)
        kind(obj, kk)
        out.append(obj)
        return obj

    hw = K_W / 2
    yf = -K_D / 2 + 0.03   # the cabinet's face
    # the cabinet: carcass, toe kick, face frame, doors, the drawer
    add(box(-hw + 0.01, hw - 0.01, yf, K_BACK - 0.01, 0.1, K_H - 0.035, 0.003), pt)
    add(box(-hw + 0.015, hw - 0.015, yf + 0.07, K_BACK - 0.01, 0.0, 0.1, 0.002), pt)
    for x0, x1, z0, z1 in ((-hw + 0.02, -0.003, 0.12, 0.69), (0.003, hw - 0.02, 0.12, 0.69)):
        if x1 < 0:
            for a, b in ((x0, (x0 + x1) / 2 - 0.002), ((x0 + x1) / 2 + 0.002, x1)):
                add(box(a, b, yf - 0.02, yf, z0, z1, 0.004), pt)
        else:
            add(box(x0, x1, yf - 0.02, yf, z0, z1, 0.004), pt)
    add(box(-hw + 0.02, -0.003, yf - 0.02, yf, 0.71, K_H - 0.05, 0.004), pt)
    add(box(0.003, hw - 0.02, yf - 0.02, yf, 0.71, K_H - 0.05, 0.004), pt)
    # hardware: knobs on the doors, the bar pull on the drawer
    for kx in (-0.33, -0.25, 0.04):
        add(lathe('knob', [(0.0, 0.0), (0.011, 0.0), (0.008, 0.012), (0.015, 0.02), (0.016, 0.028), (0.0, 0.03)], 20,
                  Matrix.Translation((kx, yf - 0.02, 0.6)) @ Matrix.Rotation(math.radians(90), 4, 'X')), stl)
    bar_y = yf - 0.02 - 0.03
    bar_z = 0.795
    add(rod((0.2, bar_y, bar_z), (0.4, bar_y, bar_z), 0.0055, 16, 'bar'), stl)
    for bx in (0.21, 0.39):
        add(rod((bx, yf - 0.02, bar_z), (bx, bar_y, bar_z), 0.005, 12, 'post'), stl)
    # the counter, its aluminum edge, the splash; a hole for the sink
    sink_out = core.rounded_rect(K_SINK[0] - 0.2, K_SINK[1] - 0.18, K_SINK[0] + 0.2, K_SINK[1] + 0.18, 0.03, steps=4)
    sink_in = core.rounded_rect(K_SINK[0] - 0.17, K_SINK[1] - 0.15, K_SINK[0] + 0.17, K_SINK[1] + 0.15, 0.025, steps=4)
    top = parts.outline_prism('counter', core.rounded_rect(-hw - 0.01, -K_D / 2, hw + 0.01, K_BACK, 0.004, steps=2), [list(reversed(sink_in))],
                              K_H - 0.035, K_H)
    add(top, ct, 0)
    add(box(-hw - 0.012, hw + 0.012, -K_D / 2 - 0.003, -K_D / 2, K_H - 0.037, K_H + 0.001, 0.001), ct, 1)
    for s in (-1, 1):
        add(box(s * (hw + 0.01) - (0.003 if s < 0 else 0.0), s * (hw + 0.01) + (0.003 if s > 0 else 0.0), -K_D / 2, K_BACK, K_H - 0.037, K_H + 0.001, 0.001), ct, 1)
    add(box(-hw - 0.01, hw + 0.01, K_BACK - 0.012, K_BACK, K_H, K_H + 0.1, 0.002), ct, 0)
    add(box(-hw - 0.01, hw + 0.01, K_BACK - 0.014, K_BACK, K_H + 0.1, K_H + 0.104, 0.001), ct, 1)
    # the sink: flange, bowl, drain
    add(parts.outline_prism('flange', sink_out, [list(reversed(sink_in))], K_H, K_H + 0.003), stl)
    bm = bmesh.new()
    rings = []
    for zz, ins in ((K_H, 0.0), (K_H - 0.02, 0.004), (K_H - 0.15, 0.012), (K_H - 0.16, 0.022)):
        rr = core.rounded_rect(K_SINK[0] - 0.17 + ins, K_SINK[1] - 0.15 + ins, K_SINK[0] + 0.17 - ins, K_SINK[1] + 0.15 - ins, max(0.025 - ins * 0.5, 0.012), steps=4)
        rings.append([bm.verts.new((x_, y_, zz)) for x_, y_ in rr])
    n_ = len(rings[0])
    for a_, b_ in zip(rings, rings[1:]):
        for q in range(n_):
            bm.faces.new((a_[q], a_[(q + 1) % n_], b_[(q + 1) % n_], b_[q]))
    bm.faces.new(rings[-1])
    bowl = core.mesh_object('bowl', bm)
    core.orient_normals(bowl, (0, 0, 1))
    bowl.data.shade_smooth()
    add(bowl, stl)
    add(lathe('drain', [(0.0, K_H - 0.162), (0.042, K_H - 0.162), (0.044, K_H - 0.158), (0.036, K_H - 0.157), (0.0, K_H - 0.157)], 32,
              Matrix.Translation((K_SINK[0], K_SINK[1], 0))), stl)
    add(lathe('strainer', [(0.0, K_H - 0.156), (0.03, K_H - 0.156), (0.03, K_H - 0.153), (0.006, K_H - 0.15), (0.0, K_H - 0.146)], 24,
              Matrix.Translation((K_SINK[0], K_SINK[1], 0))), en, 2)
    # the faucet: a deck plate, cross handles, a gooseneck
    fx, fy = K_SINK[0], K_SINK[1] + 0.2
    add(rbox('deck', (fx, fy, K_H + 0.007), (0.2, 0.05, 0.014), 0.006), stl)
    for hx in (-0.07, 0.07):
        add(lathe('valve', [(0.0, 0.0), (0.014, 0.0), (0.012, 0.03), (0.009, 0.04), (0.0, 0.042)], 20, Matrix.Translation((fx + hx, fy, K_H + 0.014))), stl)
        for a in (0, 90):
            add(rbox('cross', (fx + hx, fy, K_H + 0.056), (0.06, 0.009, 0.009), 0.0035, 2, rot=Matrix.Rotation(math.radians(a + 20), 3, 'Z')), stl)
        add(parts.disc('cap', (fx + hx, fy, K_H + 0.062), (0, 0, 1), 0.007, 0.004, 16), en, 3)
    spout = [(fx, fy, K_H + 0.014), (fx, fy, K_H + 0.13), (fx, fy - 0.03, K_H + 0.2), (fx, fy - 0.1, K_H + 0.21), (fx, fy - 0.15, K_H + 0.17), (fx, fy - 0.16, K_H + 0.14)]
    add(tube(spout, 0.0095, 16, 10, 'spout'), stl)
    # the hot plate: a white enamel body, two drip pans and coils, the knobs on its black front
    hx_, hy_ = K_HOT
    hz = K_H + 0.007
    for fx_ in (-0.2, 0.2):
        for fy_ in (-0.11, 0.11):
            add(parts.disc('foot', (hx_ + fx_, hy_ + fy_, K_H + 0.0035), (0, 0, 1), 0.012, 0.007, 12), en, 2)
    add(rbox('hp_body', (hx_, hy_, hz + 0.035), (0.46, 0.28, 0.07), 0.014, 3), en, 0)
    hpf = print_box('hp_front', (0.40, 0.003, 0.05), A, {'-y': 'hp_front'}, default='dark', r=0.001, matrix=Matrix.Translation((hx_, hy_ - 0.141, hz + 0.035)))
    add(hpf, pr)
    top_z = hz + 0.07
    for bx in (-0.11, 0.11):
        c = Vector((hx_ + bx, hy_ + 0.01, top_z))
        add(lathe('pan', [(0.0, -0.012), (0.06, -0.012), (0.085, 0.0), (0.09, 0.001), (0.0855, 0.002), (0.06, -0.009), (0.0, -0.009)], 40, Matrix.Translation(c)), stl)
        coil = []
        turns = 3.6
        for i in range(int(turns * 36) + 1):
            t = i / (turns * 36)
            a = t * turns * 2 * math.pi
            rr = 0.016 + 0.058 * t
            coil.append((c.x + rr * math.cos(a), c.y + rr * math.sin(a), top_z + 0.004))
        coil.append((c.x + 0.08, c.y - 0.02, top_z + 0.003))
        coil.append((c.x + 0.09, c.y - 0.03, top_z - 0.004))
        add(core.sweep('coil', [Vector(p) for p in coil], parts.circle(0.0045, 8)), en, 2)
        add(parts.disc('hp_knob', (hx_ + bx * 1.1, hy_ - 0.152, hz + 0.035), (0, 1, 0), 0.016, 0.02, 20), en, 2)
    add(parts.disc('pilot', (hx_, hy_ - 0.143, hz + 0.045), (0, 1, 0), 0.004, 0.004, 12), en, 4)
    # the kettle on the back burner... the right-hand one
    kc = Vector((hx_ + 0.11, hy_ + 0.01, top_z + 0.008))
    add(lathe('kettle', [(0.0, 0.0), (0.07, 0.0), (0.082, 0.006), (0.098, 0.04), (0.1, 0.065), (0.092, 0.095), (0.07, 0.125), (0.05, 0.142),
                         (0.046, 0.146), (0.0, 0.146)], 56, Matrix.Translation(kc)), en, 1)
    add(lathe('lid', [(0.0, 0.146), (0.047, 0.146), (0.044, 0.152), (0.02, 0.158), (0.0, 0.159)], 40, Matrix.Translation(kc)), en, 1)
    add(lathe('lidknob', [(0.0, 0.158), (0.008, 0.158), (0.006, 0.165), (0.012, 0.172), (0.01, 0.18), (0.0, 0.182)], 20, Matrix.Translation(kc)), en, 2)
    sp = core.catmull_rom([kc + Vector((-0.085, 0.0, 0.045)), kc + Vector((-0.12, 0.0, 0.07)), kc + Vector((-0.145, 0.0, 0.11)), kc + Vector((-0.16, 0.0, 0.135))], 8)
    add(sweep_var('spout', sp, lambda t, a: 0.017 - 0.009 * t, 16), en, 1)
    hp_ = [kc + Vector((0.07, 0.0, 0.11)), kc + Vector((0.06, 0.0, 0.2)), kc + Vector((0.0, 0.0, 0.235)), kc + Vector((-0.06, 0.0, 0.2)), kc + Vector((-0.065, 0.0, 0.12))]
    add(tube(hp_, 0.009, 12, 8, 'handle'), en, 2)
    # mugs: Dee's Riverside mug with a teabag at the back right, a brown one at the back, a diner mug in the rack
    for o_ in mug('mug', A, 'mug_riverside', matrix=mat4((0.12, 0.2, K_H), rz=-120.0)):
        add(o_, pr)
    tag_c = Vector((0.12 + 0.03, 0.2 - 0.04, K_H + 0.05))
    add(tube([(0.12 + 0.01, 0.2 - 0.005, K_H + 0.07), (0.12 + 0.035, 0.2 - 0.035, K_H + 0.098), (0.12 + 0.043, 0.2 - 0.043, K_H + 0.08), tuple(tag_c + Vector((0, 0, 0.014)))],
             0.0005, 4, 6, 'string'), rk, 3)
    add(print_box('tag', (0.022, 0.0006, 0.028), A, {'-y': 'tag', '+y': 'tag'}, default='tag', matrix=Matrix.Translation(tag_c) @ Matrix.Rotation(math.radians(-45), 4, 'Z')), pr)
    for o_ in mug('mug', A, 'mug_brown', matrix=mat4((-0.08, 0.22, K_H), rz=150.0)):
        add(o_, pr)
    tin = lathe('tin', [(0.0, 0.0), (0.05, 0.0), (0.051, 0.003), (0.051, 0.127), (0.05, 0.13), (0.0, 0.13)], 48)

    def tin_fn(P, N):
        side = np.abs(N[:, 2]) < 0.5
        uv = A.uv('tea', np.full(len(P), 0.5), np.full(len(P), 0.9))
        ang = (np.arctan2(P[side, 1], P[side, 0]) / (2 * np.pi) + 0.25) % 1.0
        uv[side] = A.uv('tea', ang, P[side, 2] / 0.13)
        return uv
    pat_faces(tin, tin_fn)
    tin.data.transform(mat4((0.5, 0.2, K_H), rz=-80.0))
    add(tin, pr)
    add(lathe('tin_lid', [(0.0, 0.128), (0.0525, 0.128), (0.0525, 0.142), (0.05, 0.145), (0.0, 0.145)], 48, Matrix.Translation((0.5, 0.2, K_H))), stl)
    # the dish rack: a drip tray, a coated wire basket, two plates, a bowl, the diner mug, cutlery
    rx_, ry_ = -0.02, -0.11
    add(rbox('tray', (rx_, ry_, K_H + 0.007), (0.31, 0.3, 0.014), 0.006), rk, 1)
    wires = []
    zb = K_H + 0.022
    for i in range(15):
        x_ = rx_ - 0.135 + i * 0.27 / 14
        wires.append([(x_, ry_ - 0.13, zb), (x_, ry_ + 0.13, zb)])
    for yy in (-0.13, 0.13):
        wires.append([(rx_ - 0.14, ry_ + yy, zb), (rx_ - 0.14, ry_ + yy, zb + 0.09), (rx_ + 0.14, ry_ + yy, zb + 0.09), (rx_ + 0.14, ry_ + yy, zb)])
    for xx in (-0.14, 0.14):
        wires.append([(rx_ + xx, ry_ - 0.13, zb + 0.09), (rx_ + xx, ry_ + 0.13, zb + 0.09)])
        wires.append([(rx_ + xx, ry_ - 0.13, zb), (rx_ + xx, ry_ + 0.13, zb)])
    for i in range(8):
        y_ = ry_ - 0.02 + i * 0.018
        wires.append([(rx_ - 0.1, y_, zb), (rx_ - 0.1, y_, zb + 0.13), (rx_ - 0.07, y_, zb + 0.15), (rx_ - 0.04, y_, zb + 0.13), (rx_ - 0.04, y_, zb)])
    for w_ in wires:
        add(tube(w_, 0.0022, 6, 2, 'wire'), rk, 0)
    for i, y_ in enumerate((ry_ + 0.007, ry_ + 0.043)):
        pl = lathe('plate', [(0.0, 0.0), (0.06, 0.0), (0.075, 0.008), (0.11, 0.014), (0.112, 0.018), (0.108, 0.018), (0.073, 0.012), (0.0, 0.004)], 48,
                   Matrix.Translation((rx_ - 0.07, y_, zb + 0.112)) @ Matrix.Rotation(math.radians(90 + (i - 0.5) * 8), 4, 'X'))
        add(pl, rk, 1)
    add(lathe('bowl', [(0.0, 0.0), (0.035, 0.0), (0.06, 0.03), (0.075, 0.06), (0.072, 0.062), (0.056, 0.032), (0.0, 0.006)], 40,
              mat4((rx_ + 0.07, ry_ - 0.05, zb + 0.06), rx=150.0, rz=20.0)), rk, 1)
    for o_ in mug('mug', A, 'mug_diner', matrix=mat4((rx_ + 0.08, ry_ + 0.08, zb + 0.095), rz=40.0) @ Matrix.Rotation(math.radians(180), 4, 'X')):
        add(o_, pr)
    for i, (dx, dy) in enumerate(((0.0, 0.0), (0.012, 0.006))):
        add(rbox('fork' if i == 0 else 'spoon', (rx_ + 0.11 + dx, ry_ - 0.1 + dy, zb + 0.07), (0.008, 0.004, 0.17), 0.0015, rot=Matrix.Rotation(math.radians(12 - i * 15), 3, 'Y')), rk, 2)
    add(box(rx_ + 0.09, rx_ + 0.14, ry_ - 0.125, ry_ - 0.075, zb, zb + 0.06, 0.003), rk, 0)
    # the calendar on its nail
    cal_c = (0.12, K_BACK - 0.0012, 1.32)
    cal = print_box('calendar', (0.30, 0.0016, 0.46), A, {'-y': 'calendar', '+y': 'white'}, default='white', matrix=Matrix.Translation(cal_c) @ Matrix.Rotation(math.radians(1.5), 4, 'Y'))
    P = verts_np(cal)
    # the pages bow off the wall a little at the bottom
    P[:, 1] -= np.clip((1.32 - P[:, 2]) / 0.23, 0, 1) ** 2 * 0.006
    set_verts(cal, P)
    add(cal, pr)
    add(parts.disc('nail', (0.12 - 0.006, K_BACK - 0.008, 1.32 + 0.21), (0, 1, 0), 0.0028, 0.012, 10), stl)
    # the towel over the drawer's bar
    colliders = [o_ for o_ in out if o_.name.startswith(('bar', 'box'))]
    towel = drape('towel', (0.3, 0.6), 0.015, mat4((0.3, bar_y - 0.005, bar_z + 0.04)), colliders,
                  frames=70, mass=0.15, bend=0.05, gap=0.0025, friction=20.0)
    thicken(towel, 0.003)
    me = towel.data
    pl_ = me.uv_layers['pat']
    uvt = np.empty(len(pl_.data) * 2, np.float32)
    pl_.data.foreach_get('uv', uvt)
    pl_.data.foreach_set('uv', (uvt.reshape(-1, 2) * np.array([0.3, 0.6], np.float32)).ravel())
    add(towel, tw)
    return [finish(out, 'SM_DR_Kitchenette', [pt, ct, stl, en, pr, rk, tw], angle=40.0)]


# ================================================================== SM_DR_Window

W_X = 0.43                # the opening's half width
W_Z = (0.826, 2.30)       # the stool's top to the head jamb
W_SILL = 0.826
W_LOWER = (0.826, 1.585)  # the lower sash, inner track
W_UPPER = (1.545, 2.285)  # the upper sash, outer track
W_Y_LOW = (0.058, 0.098)
W_Y_UP = (0.112, 0.152)


def cactus(base, rng):
    """A columnar cactus (8 ribs, about 17 cm) with a small arm, spined at its areoles, in a clay pot on a saucer at base.
    Returns [(object, dr_k)]: 0 body, 1 spines, 2 pot, 3 saucer, 4 soil."""
    out = []
    pot = lathe('cpot', [(0.0, 0.0), (0.036, 0.0), (0.038, 0.005), (0.046, 0.065), (0.051, 0.068), (0.051, 0.08), (0.046, 0.08), (0.044, 0.07),
                         (0.036, 0.01), (0.0, 0.01)], 40, Matrix.Translation(base))
    out.append((pot, 2))
    out.append((lathe('csaucer', [(0.0, 0.0), (0.05, 0.0), (0.057, 0.01), (0.054, 0.011), (0.046, 0.003), (0.0, 0.003)], 40, Matrix.Translation(base)), 3))
    out.append((lathe('csoil', [(0.0, 0.07), (0.044, 0.068), (0.043, 0.06), (0.0, 0.06)], 24, Matrix.Translation(base)), 4))

    def column(c0, height, R, nrib=8, tilt=None):
        rings = []
        nz, na = 18, 64
        for j in range(nz + 1):
            t = j / nz
            rr = R * (1.0 if t < 0.8 else math.sqrt(max(1 - ((t - 0.8) / 0.2) ** 2, 0.0)))
            rr = max(rr, 0.0015)
            zz = height * t
            ring = []
            for i in range(na):
                a = 2 * math.pi * i / na
                rib = 1.0 + 0.13 * max(math.cos(a * nrib), -0.4)
                ring.append(Vector((rr * rib * math.cos(a), rr * rib * math.sin(a), zz)))
            rings.append(ring)
        ob = core.loft('cactus', [[tuple(p) for p in r] for r in rings])
        M = Matrix.Translation(c0) @ (tilt if tilt is not None else Matrix())
        ob.data.transform(M)
        ob.data.shade_smooth()
        spines = []
        for i in range(nrib):
            a = 2 * math.pi * i / nrib
            for j in range(int(height / 0.014)):
                zz = 0.012 + j * 0.014 + (0.007 if i % 2 else 0.0)
                if zz > height * 0.92:
                    continue
                t = zz / height
                rr = R * (1.0 if t < 0.8 else math.sqrt(max(1 - ((t - 0.8) / 0.2) ** 2, 0.0))) * 1.13
                p = Vector((rr * math.cos(a), rr * math.sin(a), zz))
                out_d = Vector((math.cos(a), math.sin(a), 0.25)).normalized()
                for s_ in range(3):
                    d = (out_d + Vector((rng.uniform(-0.5, 0.5), rng.uniform(-0.5, 0.5), rng.uniform(-0.3, 0.5)))).normalized()
                    L = rng.uniform(0.007, 0.012)
                    sp = lathe('spine', [(0.0, 0.0), (0.0006, 0.0), (0.0, L)], 4, Matrix.Translation(p) @ Vector((0, 0, 1)).rotation_difference(d).to_matrix().to_4x4())
                    sp.data.transform(M)
                    spines.append(sp)
        return ob, spines
    body, sp1 = column(base + Vector((0, 0, 0.066)), 0.17, 0.026)
    arm, sp2 = column(base + Vector((0.018, 0.0, 0.12)), 0.06, 0.013, 6, Matrix.Rotation(math.radians(35), 4, 'Y'))
    out += [(body, 0), (arm, 0)] + [(s_, 1) for s_ in sp1 + sp2]
    return out


def build_window():
    """SM_DR_Window: the inside of a painted double-hung tenement window. The rough opening is 0.86 x 1.48 m
    (x +-0.43, z 0.826 to 2.30) in a wall whose room face is y = 0 (the jambs run 0.16 deep into +Y); casing to
    x +-0.55 and z 2.43, a stool (sill) 8.5 cm into the room with a cactus on it, a 2-over-2 lower sash (inner) and
    upper sash, a brass sash lock and lifts, a yellowed roller shade pulled 30 cm down. SM_DR_Window_Glass is the
    four panes (UV 0..1 each), material dr_window_glass."""
    def grime(m, sep):
        return box_mask(m, sep, (-0.3, None, W_SILL - 0.01), (0.3, None, W_SILL + 0.04), 0.03)
    pt = old_paint('drw_paint', 0xe4ddca, under1=0x9ab09a, under2=0xd8c8a0, base=0x6a4a2c, crackle=0.22, grime=grime, gloss=0.42)
    mt = brass('drw_brass', 0x8a6a32, 0.75, 0.38)
    pm = plant_material('drw_plant', 0xb0623a, 'terracotta')
    sh = core.Mat('drw_shade')
    tc, sep = object_coords(sh)
    o = tc.outputs['Object']
    k = attr(sh, 'dr_k')
    stain = ramp(sh, noise(sh, o, scale=6.0, detail=4.0), 0.55, 0.72)
    vin = sh.mix(ramp(sh, sep.outputs['Z'], 1.9, 2.25), H(0xd8ccaa), H(0xc4b48a))
    vin = sh.mix(sh.math('MULTIPLY', stain, 0.5), vin, H(0xa08a5a))
    col = sh.mix(is_k(sh, k, 1), vin, H(0x6a4a2c))
    col = sh.mix(is_k(sh, k, 2), col, H(0xe8e2d0))
    sh.set('Base Color', col)
    sh.set('Roughness', 0.7)
    set_bump(sh, noise(sh, scaled(sh, sep, 2000.0, 2000.0, 300.0), detail=1.0), 0.15, 0.0003)
    out = []

    def add(obj, mat, kk=0):
        core.assign(obj, mat)
        kind(obj, kk)
        out.append(obj)
        return obj

    z0, z1 = W_Z
    # casing, its head and cap, the stool and apron
    for s in (-1, 1):
        add(box(min(s * W_X, s * (W_X + 0.09)), max(s * W_X, s * (W_X + 0.09)), -0.022, 0.0, z0, z1, 0.003), pt)
    add(box(-W_X - 0.1, W_X + 0.1, -0.022, 0.0, z1, z1 + 0.1, 0.003), pt)
    add(box(-W_X - 0.125, W_X + 0.125, -0.042, 0.0, z1 + 0.1, z1 + 0.125, 0.004), pt)
    add(box(-W_X - 0.11, W_X + 0.11, -0.032, 0.0, z1 + 0.094, z1 + 0.104, 0.002), pt)
    add(box(-W_X - 0.13, W_X + 0.13, -0.085, 0.075, W_SILL - 0.026, W_SILL, 0.006), pt)
    add(box(-W_X - 0.08, W_X + 0.08, -0.02, 0.0, W_SILL - 0.11, W_SILL - 0.026, 0.003), pt)
    # jambs and head jamb; stops and parting beads
    for s in (-1, 1):
        add(box(s * W_X - (0.02 if s < 0 else 0.0), s * W_X + (0.02 if s > 0 else 0.0), 0.0, 0.17, W_SILL, z1, 0.002), pt)
        add(box(min(s * (W_X - 0.018), s * W_X), max(s * (W_X - 0.018), s * W_X), 0.04, 0.056, W_SILL, z1, 0.002), pt)
        add(box(min(s * (W_X - 0.012), s * W_X), max(s * (W_X - 0.012), s * W_X), 0.1, 0.11, W_SILL, z1, 0.0015), pt)
    add(box(-W_X - 0.02, W_X + 0.02, 0.0, 0.17, z1 - 0.02, z1, 0.002), pt)
    # sashes: stiles, rails, a muntin each
    sx0, sx1 = -(W_X - 0.018), W_X - 0.018
    for (za, zb), (ya, yb), bottom, top in ((W_LOWER, W_Y_LOW, 0.075, 0.034), (W_UPPER, W_Y_UP, 0.034, 0.05)):
        for s in (-1, 1):
            add(box(min(s * sx1, s * (sx1 - 0.05)), max(s * sx1, s * (sx1 - 0.05)), ya, yb, za, zb, 0.0025), pt)
        add(box(sx0 + 0.05, sx1 - 0.05, ya, yb, za, za + bottom, 0.0025), pt)
        add(box(sx0 + 0.05, sx1 - 0.05, ya, yb, zb - top, zb, 0.0025), pt)
        add(box(-0.011, 0.011, ya + 0.006, yb - 0.006, za + bottom, zb - top, 0.002), pt)
    # the shade: its roller, the cloth pulled down 30 cm, the hem slat, a pull ring
    add(rod((-W_X + 0.004, 0.03, z1 - 0.035), (W_X - 0.004, 0.03, z1 - 0.035), 0.019, 20, 'roller'), sh, 2)
    shade_bot = z1 - 0.34
    cloth = box(-W_X + 0.012, W_X - 0.012, 0.011, 0.0125, shade_bot, z1 - 0.035, 0.0)
    P = verts_np(cloth)
    P[:, 1] += 0.003 * np.sin(P[:, 0] * 14.0) * np.clip((z1 - 0.04 - P[:, 2]) / 0.3, 0, 1)
    set_verts(cloth, P)
    add(cloth, sh, 0)
    add(box(-W_X + 0.014, W_X - 0.014, 0.004, 0.016, shade_bot - 0.022, shade_bot + 0.002, 0.004), sh, 1)
    add(tube([(0.0, 0.01, shade_bot - 0.02), (0.0, 0.008, shade_bot - 0.07)], 0.0012, 6, 2, 'pull'), sh, 1)
    ring = lathe('pullring', [(0.016 + 0.0028 * math.cos(2 * math.pi * t / 8), 0.0028 * math.sin(2 * math.pi * t / 8)) for t in range(9)], 24,
                 Matrix.Translation((0.0, 0.008, shade_bot - 0.087)) @ Matrix.Rotation(math.radians(90), 4, 'X'))
    add(ring, sh, 2)
    # hardware: the sash lock on the meeting rails, two lifts on the lower sash
    add(rbox('lock_base', (0.0, W_Y_LOW[0] + 0.02, W_LOWER[1] + 0.003), (0.06, 0.025, 0.006), 0.002), mt)
    add(rbox('lock_cam', (0.004, W_Y_LOW[0] + 0.024, W_LOWER[1] + 0.009), (0.045, 0.018, 0.005), 0.003, rot=Matrix.Rotation(math.radians(-12), 3, 'Z')), mt)
    add(rbox('keeper', (0.0, W_Y_UP[0] + 0.012, W_UPPER[0] + 0.034 + 0.003), (0.05, 0.02, 0.006), 0.002), mt)
    for lx in (-0.2, 0.2):
        add(rbox('lift_plate', (lx, W_Y_LOW[0] - 0.002, W_LOWER[0] + 0.045), (0.04, 0.004, 0.022), 0.0015), mt)
        add(tube([(lx - 0.015, W_Y_LOW[0] - 0.004, W_LOWER[0] + 0.05), (lx - 0.012, W_Y_LOW[0] - 0.02, W_LOWER[0] + 0.045), (lx + 0.012, W_Y_LOW[0] - 0.02, W_LOWER[0] + 0.045),
                  (lx + 0.015, W_Y_LOW[0] - 0.004, W_LOWER[0] + 0.05)], 0.0035, 10, 6, 'lift'), mt)
    # the cactus on the sill
    rng = random.Random(41)
    for obj, kk in cactus(Vector((0.24, -0.015, W_SILL)), rng):
        add(obj, pm, {0: 6, 1: 5, 2: 3, 3: 4, 4: 2}[kk])
    win = finish(out, 'SM_DR_Window', [pt, mt, pm, sh], angle=40.0)
    # the glass: four panes, each its own 0..1
    panes = []
    for (za, zb), (ya, yb), bottom, top in ((W_LOWER, W_Y_LOW, 0.075, 0.034), (W_UPPER, W_Y_UP, 0.034, 0.05)):
        yc = (ya + yb) / 2
        for xa, xb in ((sx0 + 0.05, -0.011), (0.011, sx1 - 0.05)):
            bm = bmesh.new()
            uvl = bm.loops.layers.uv.new('UVMap')
            vs = [bm.verts.new((xa, yc, za + bottom)), bm.verts.new((xb, yc, za + bottom)), bm.verts.new((xb, yc, zb - top)), bm.verts.new((xa, yc, zb - top))]
            f = bm.faces.new(vs)
            for l, uv in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
                l[uvl].uv = uv
            p_ = core.mesh_object('pane', bm)
            core.orient_normals(p_, (0, -1, 0))
            panes.append(p_)
    gl = core.join(panes, 'SM_DR_Window_Glass')
    gl.name = 'SM_DR_Window_Glass'
    gm = core.Mat('dr_window_glass')
    gm.set('Base Color', H(0x0c0f13))
    gm.set('Roughness', 0.05)
    gm.set('Specular IOR Level', 0.6)
    gm.m.use_backface_culling = False
    core.assign(gl, gm)
    gl.data.shade_flat()
    return [win, gl]


# ================================================================== SM_DR_Door

D_W, D_H = 0.915, 2.035   # the opening
D_LEAF = (0.4525, 0.012, 2.03, 0.002, 0.046)   # half width, bottom, top, front y, back y


def build_door():
    """SM_DR_Door: Dee's apartment door from inside, in its frame: the opening 0.915 x 2.035 m in a wall whose room
    face is y = 0 (jambs 0.15 deep into +Y), casing to x +-0.55 and z 2.15, a marble saddle. A six-panel door in
    hallway-brown enamel (cream trim), hinged on the right (+X) and opening into the room; a brass knob and deadbolt
    thumb-turn on the left, the chain hanging loose from its plate on the casing, the peephole at 1.52 m. Scuffed at
    the foot, greasy round the knob."""
    def grime(m, sep):
        x, z = sep.outputs['X'], sep.outputs['Z']
        knob = m.math('MULTIPLY', smooth_less(m, dist3(m, sep, (-0.39, 0.0, 0.95)), 0.16, 0.08), 1.0)
        kick = m.math('MULTIPLY', m.math('SUBTRACT', 1.0, ramp(m, z, 0.12, 0.3)), m.math('SUBTRACT', 1.0, ramp(m, m.math('ABSOLUTE', x), 0.4, 0.46)))
        return m.math('MAXIMUM', knob, m.math('MULTIPLY', kick, 0.9))
    pt = old_paint('drd_paint', 0x3a1e16, under1=0x6a7a5a, under2=0xd8ccae, base=0x6a4a2c, grime=grime, gloss=0.3, colors={1: 0xe0d8c4})
    mt = core.Mat('drd_metal')
    tc, sep = object_coords(mt)
    k = attr(mt, 'dr_k')
    o = tc.outputs['Object']
    patch = ramp(mt, noise(mt, o, scale=20.0, detail=4.0), 0.4, 0.7)
    rub = ramp(mt, convex_edges(mt, tc, distance=0.0015, samples=12, breakup_scale=120.0, gain=4.0), 0.2, 0.6)
    brass_c = mt.mix(rub, mt.mix(patch, H(0x9a7838), H(0x4e3c1c)), H(0xe0c070))
    steel_c = mt.mix(patch, H(0xa8a8a4), H(0x6a6a64))
    col = mt.mix(is_k(mt, k, 1), brass_c, steel_c)
    col = mt.mix(is_k(mt, k, 2), col, H(0x050608))
    mt.set('Base Color', col)
    mt.set('Metallic', mix_float(mt, is_k(mt, k, 2), 1.0, 0.0))
    mt.set('Roughness', mix_float(mt, rub, mt.math('ADD', 0.3, mt.math('MULTIPLY', patch, 0.25)), 0.15))
    st = core.Mat('drd_stone')
    tc2, sep2 = object_coords(st)
    vein = ramp(st, noise(st, scaled(st, sep2, 30.0, 6.0, 30.0), detail=6.0, distortion=4.0), 0.55, 0.62)
    st_c = st.mix(vein, H(0xd8d4cc), H(0x8a8478))
    worn = st.math('MULTIPLY', smooth_less(st, st.math('ABSOLUTE', sep2.outputs['X']), 0.25, 0.15), ramp(st, noise(st, tc2.outputs['Object'], scale=10.0, detail=3.0), 0.3, 0.7))
    st.set('Base Color', st.mix(st.math('MULTIPLY', worn, 0.4), st_c, H(0xa09888)))
    st.set('Roughness', st.math('ADD', 0.25, st.math('MULTIPLY', worn, 0.3)))
    out = []

    def add(obj, mat, kk=0):
        core.assign(obj, mat)
        kind(obj, kk)
        out.append(obj)
        return obj

    hx = D_W / 2
    # trim (k 1): casing, head and cap, jambs, stops
    for s in (-1, 1):
        add(box(min(s * hx, s * (hx + 0.09)), max(s * hx, s * (hx + 0.09)), -0.02, 0.0, 0.0, D_H, 0.003), pt, 1)
        add(box(min(s * hx, s * (hx + 0.02)), max(s * hx, s * (hx + 0.02)), 0.0, 0.15, 0.0, D_H, 0.002), pt, 1)
        add(box(min(s * (hx - 0.016), s * hx), max(s * (hx - 0.016), s * hx), D_LEAF[4] + 0.001, D_LEAF[4] + 0.016, 0.012, D_H - 0.016, 0.0015), pt, 1)
    add(box(-hx - 0.1, hx + 0.1, -0.02, 0.0, D_H, D_H + 0.095, 0.003), pt, 1)
    add(box(-hx - 0.115, hx + 0.115, -0.036, 0.0, D_H + 0.095, D_H + 0.118, 0.004), pt, 1)
    add(box(-hx - 0.02, hx + 0.02, 0.0, 0.15, D_H - 0.02, D_H, 0.002), pt, 1)
    add(box(-hx + 0.016, hx - 0.016, D_LEAF[4] + 0.001, D_LEAF[4] + 0.016, D_H - 0.032, D_H - 0.016, 0.0015), pt, 1)
    # the saddle
    add(box(-hx, hx, -0.012, 0.15, 0.0, 0.012, 0.004), st)
    # the leaf: its core, the stile-and-rail face with six openings, the raised panels in them
    lw, zb, zt, yf, yb = D_LEAF
    add(box(-lw, lw, yf + 0.01, yb, zb, zt, 0.002), pt, 0)
    rails = [(zb, 0.24), (0.84, 1.0), (1.66, 1.78), (1.91, zt)]
    panels = [(0.24, 0.84), (1.0, 1.66), (1.78, 1.91)]
    holes = []
    for za, zz in panels:
        for xa, xb_ in ((-lw + 0.12, -0.06), (0.06, lw - 0.12)):
            holes.append(list(reversed([(xa, za), (xb_, za), (xb_, zz), (xa, zz)])))
    face = parts.outline_prism('face', [(-lw, zb), (lw, zb), (lw, zt), (-lw, zt)], holes, 0.0, 0.01)
    face.data.transform(Matrix.Translation((0, yf, 0)) @ Matrix.Rotation(math.radians(90), 4, 'X') @ Matrix.Diagonal((1, 1, -1, 1)))
    # The mirror turned the plate inside out. Reverse it outright: the plate's caps aren't stitched to its walls, so
    # recalculating normals (orient_normals) left the room-side face pointing into the door, culled in game.
    reverse_faces(face)
    add(face, pt, 0)
    for za, zz in panels:
        for xa, xb_ in ((-lw + 0.12, -0.06), (0.06, lw - 0.12)):
            rings = []
            for ins, yy in ((0.003, yf + 0.013), (0.003, yf + 0.0075), (0.034, yf + 0.0032), (0.036, yf + 0.003)):
                rings.append([(xa + ins, yy, za + ins), (xb_ - ins, yy, za + ins), (xb_ - ins, yy, zz - ins), (xa + ins, yy, zz - ins)])
            add(core.loft('panel', rings), pt, 0)
    # hinges on the right
    for hz in (0.25, 1.0, 1.8):
        add(rod((lw + 0.004, yf - 0.002, hz - 0.045), (lw + 0.004, yf - 0.002, hz + 0.045), 0.0068, 16, 'knuckle'), mt, 0)
        add(rbox('leaf', (lw + 0.006, yf + 0.004, hz), (0.004, 0.012, 0.088), 0.0008), mt, 0)
        add(lathe('pin', [(0.0, 0.0), (0.006, 0.0), (0.004, 0.004), (0.0, 0.005)], 12, Matrix.Translation((lw + 0.004, yf - 0.002, hz + 0.045))), mt, 0)
    # the knob, the deadbolt's thumb-turn, the peephole
    kx = -lw + 0.065
    add(lathe('rose', [(0.0, 0.0), (0.03, 0.0), (0.03, 0.003), (0.022, 0.009), (0.0, 0.01)], 32, Matrix.Translation((kx, yf, 0.94)) @ Matrix.Rotation(math.radians(90), 4, 'X')), mt, 0)
    add(lathe('knob', [(0.0, 0.0), (0.009, 0.0), (0.009, 0.03), (0.02, 0.038), (0.028, 0.052), (0.027, 0.064), (0.018, 0.071), (0.0, 0.073)], 32,
              Matrix.Translation((kx, yf, 0.94)) @ Matrix.Rotation(math.radians(90), 4, 'X')), mt, 0)
    add(lathe('db_rose', [(0.0, 0.0), (0.032, 0.0), (0.031, 0.004), (0.026, 0.011), (0.0, 0.012)], 32, Matrix.Translation((kx, yf, 1.11)) @ Matrix.Rotation(math.radians(90), 4, 'X')), mt, 0)
    add(rbox('thumb', (kx, yf - 0.022, 1.11), (0.012, 0.022, 0.04), 0.005, 3, rot=Matrix.Rotation(math.radians(8), 3, 'Y')), mt, 0)
    add(lathe('peep', [(0.0, 0.0), (0.012, 0.0), (0.011, 0.004), (0.007, 0.006), (0.0, 0.006)], 24, Matrix.Translation((0.0, yf, 1.52)) @ Matrix.Rotation(math.radians(90), 4, 'X')), mt, 0)
    add(parts.disc('lens', (0.0, yf - 0.0055, 1.52), (0, 1, 0), 0.0055, 0.002, 16), mt, 2)
    # the chain: its slide track on the door, its plate on the casing, the chain hanging loose
    add(rbox('track', (-lw + 0.08, yf - 0.003, 1.43), (0.11, 0.006, 0.022), 0.002), mt, 1)
    add(rbox('track_slot', (-lw + 0.08, yf - 0.0062, 1.43), (0.09, 0.001, 0.005), 0.0), mt, 2)
    anchor = Vector((-hx - 0.045, -0.024, 1.43))
    add(rbox('anchor', tuple(anchor + Vector((0, 0.002, 0))), (0.026, 0.004, 0.052), 0.002), mt, 1)
    n_links = 13
    pts = []
    for i in range(n_links + 1):
        t = i / n_links
        pts.append(anchor + Vector((0.006 * t, -0.006 - 0.004 * math.sin(t * math.pi), -0.012 - 0.17 * t)))
    for i in range(n_links):
        a, b = pts[i], pts[i + 1]
        c = (a + b) / 2
        d = (b - a).normalized()
        link = lathe('link', [(0.0045 + 0.0011 * math.cos(2 * math.pi * t_ / 8), 0.0011 * math.sin(2 * math.pi * t_ / 8)) for t_ in range(9)], 12)
        # a ring stretched along the chain, every other one turned a quarter
        Mlink = (Matrix.Translation(c) @ Vector((1, 0, 0)).rotation_difference(d).to_matrix().to_4x4() @ Matrix.Rotation(math.radians(90 * (i % 2)), 4, 'X')
                 @ Matrix.Diagonal((1.6, 1.0, 1.0, 1.0)))
        link.data.transform(Mlink)
        add(link, mt, 1)
    add(lathe('chain_knob', [(0.0, 0.0), (0.004, 0.0), (0.008, 0.006), (0.008, 0.012), (0.004, 0.016), (0.0, 0.017)], 16,
              Matrix.Translation(pts[-1] - Vector((0, 0, 0.017)))), mt, 1)
    return [finish(out, 'SM_DR_Door', [pt, mt, st], angle=40.0)]


# ================================================================== SM_Umbrella

U_R = 0.5          # the canopy's radius
U_TOP = 0.92       # the canopy's crown, over the grip
U_SPH = 0.62       # the canopy is a spherical cap of this radius: 0.25 m deep, its rim 54 degrees down
U_RIBS = 8


def u_profile(r):
    return U_TOP - (U_SPH - math.sqrt(U_SPH * U_SPH - float(r) * float(r)))


def build_umbrella():
    """SM_Umbrella: an open black stick umbrella, 1.0 m across, for the street's people. The origin is the middle of the
    straight grip (hold it there), the shaft and canopy up +Z: the rim about 0.67 m over the hand, the tip at 0.99; the
    J hook below the hand opens toward +X. Eight ribs with plastic tips, stretchers, the runner; the nylon is wet
    (beaded, glossy), a closure strap hangs from one panel."""
    cm = core.Mat('dru_canopy')
    tc, sep = object_coords(cm)
    o = tc.outputs['Object']
    vor = cm.node('ShaderNodeTexVoronoi', Scale=95.0)
    cm.link(o, vor.inputs['Vector'])
    drop = cm.math('MULTIPLY', smooth_less(cm, vor.outputs['Distance'], 0.22, 0.06), ramp(cm, noise(cm, o, scale=95.0, detail=0.0), 0.35, 0.5))
    streak = ramp(cm, noise(cm, scaled(cm, sep, 40.0, 40.0, 3.0), detail=3.0), 0.6, 0.72)
    weave = noise(cm, o, scale=2600.0, detail=1.0)
    col = cm.mix(cm.math('MULTIPLY', weave, 0.3), H(0x0b0b0d), H(0x16161a))
    col = cm.mix(cm.math('MULTIPLY', drop, 0.6), col, H(0x050506))
    cm.set('Base Color', col)
    rough = cm.math('ADD', 0.14, cm.math('MULTIPLY', weave, 0.08))
    rough = mix_float(cm, drop, rough, 0.03)
    rough = mix_float(cm, cm.math('MULTIPLY', streak, 0.6), rough, 0.08)
    cm.set('Roughness', rough)
    cm.set('Specular IOR Level', 0.6)
    cm.set('Coat Weight', 0.5)
    cm.set('Coat Roughness', 0.06)
    set_bump(cm, cm.math('ADD', cm.math('MULTIPLY', drop, 1.2), cm.math('MULTIPLY', weave, 0.2)), 0.35, 0.0012)
    fr = core.Mat('dru_frame')
    tc2, sep2 = object_coords(fr)
    k = attr(fr, 'dr_k')
    fr.set('Base Color', fr.mix(is_k(fr, k, 2), fr.mix(is_k(fr, k, 1), H(0x1a1a1c), H(0x0c0c0c)), H(0xb8bab8)))
    fr.set('Metallic', mix_float(fr, is_k(fr, k, 1), 0.85, 0.0))
    fr.set('Roughness', mix_float(fr, is_k(fr, k, 1), 0.35, 0.25))
    hd = core.Mat('dru_handle')
    tc3, sep3 = object_coords(hd)
    hn = noise(hd, tc3.outputs['Object'], scale=1800.0, detail=2.0)
    hd.set('Base Color', hd.mix(hd.math('MULTIPLY', hn, 0.4), H(0x121212), H(0x222222)))
    hd.set('Roughness', hd.math('ADD', 0.45, hd.math('MULTIPLY', hn, 0.1)))
    set_bump(hd, hn, 0.2, 0.0002)
    out = []

    def add(obj, mat, kk=0):
        core.assign(obj, mat)
        kind(obj, kk)
        out.append(obj)
        return obj

    # the canopy: a polar grid, sagging between the ribs, its edge scalloped
    nr, nper = 16, 10
    na = U_RIBS * nper
    bm = bmesh.new()
    rings = []
    for j in range(nr + 1):
        t = 0.03 + 0.97 * j / nr
        ring = []
        for i in range(na):
            a = 2 * math.pi * i / na
            f = (i % nper) / nper
            rmax = U_R * (1 - 0.035 * math.sin(math.pi * f))
            r = t * rmax
            z = u_profile(r) - 0.018 * (r / U_R) ** 1.2 * math.sin(math.pi * f)
            ring.append(bm.verts.new((r * math.cos(a), r * math.sin(a), z)))
        rings.append(ring)
    for a_, b_ in zip(rings, rings[1:]):
        for i in range(na):
            bm.faces.new((a_[i], a_[(i + 1) % na], b_[(i + 1) % na], b_[i]))
    cap = bm.faces.new(rings[0])
    canopy = core.mesh_object('canopy', bm)
    core.orient_normals(canopy, (0, 0, 1))
    canopy.data.shade_smooth()
    add(canopy, cm)
    add(lathe('crown', [(0.0, U_TOP + 0.012), (0.03, U_TOP + 0.004), (0.034, U_TOP - 0.002), (0.0, U_TOP - 0.004)], 24), cm)
    # shaft, tip, the top notch, the runner
    add(rod((0, 0, 0.055), (0, 0, U_TOP + 0.02), 0.0055, 12, 'shaft'), fr, 1)
    add(lathe('tip', [(0.0, U_TOP + 0.01), (0.007, U_TOP + 0.01), (0.006, U_TOP + 0.05), (0.004, U_TOP + 0.065), (0.0, U_TOP + 0.07)], 12), fr, 2)
    add(lathe('notch', [(0.0, U_TOP - 0.035), (0.012, U_TOP - 0.035), (0.014, U_TOP - 0.02), (0.012, U_TOP - 0.008), (0.0, U_TOP - 0.008)], 16), fr, 0)
    zr = 0.62   # the runner, open: the stretchers about a third of the ribs
    add(lathe('runner', [(0.0, zr - 0.04), (0.009, zr - 0.04), (0.012, zr - 0.02), (0.014, zr + 0.01), (0.011, zr + 0.02), (0.0, zr + 0.02)], 16), fr, 0)
    # ribs under the fabric, their tips, the stretchers
    for i in range(U_RIBS):
        a = 2 * math.pi * i / U_RIBS
        d = Vector((math.cos(a), math.sin(a), 0.0))
        rib = [d * r + Vector((0, 0, u_profile(r) - 0.005)) for r in np.linspace(0.012, U_R * 1.005, 14)]
        add(tube(rib, 0.0018, 6, 2, 'rib'), fr, 0)
        tip_p = d * (U_R * 1.005) + Vector((0, 0, u_profile(U_R) - 0.005))
        tang = (rib[-1] - rib[-2]).normalized()
        add(lathe('rib_tip', [(0.0, 0.0), (0.0032, 0.0), (0.0035, 0.012), (0.002, 0.02), (0.0, 0.021)], 10,
                  Matrix.Translation(tip_p) @ Vector((0, 0, 1)).rotation_difference(tang).to_matrix().to_4x4()), fr, 1)
        mid = d * 0.25 + Vector((0, 0, u_profile(0.25) - 0.006))
        add(tube([d * 0.011 + Vector((0, 0, zr + 0.01)), mid], 0.0015, 6, 1, 'stretcher'), fr, 0)
    # the closure strap hanging off one panel's edge
    a = 2 * math.pi * 0.5 / U_RIBS
    sp = Vector((math.cos(a), math.sin(a), 0.0)) * (U_R * 0.96)
    strap = [sp + Vector((0, 0, u_profile(U_R * 0.96) - 0.012)), sp * 1.01 + Vector((0, 0, u_profile(U_R) - 0.06)), sp * 1.0 + Vector((0, 0, u_profile(U_R) - 0.11))]
    s_ = core.sweep('strap', [Vector(p) for p in core.catmull_rom(strap, 6)], [(-0.012, -0.0012), (0.012, -0.0012), (0.012, 0.0012), (-0.012, 0.0012)])
    add(s_, cm)
    # the grip and the J hook
    hook = [(0.0, 0.0, 0.065), (0.0, 0.0, -0.06), (0.012, 0.0, -0.1), (0.045, 0.0, -0.118), (0.078, 0.0, -0.1), (0.09, 0.0, -0.068), (0.087, 0.0, -0.05)]
    add(sweep_var('grip', core.catmull_rom(hook, 8), lambda t, a_: 0.0135 if t < 0.85 else 0.0135 * (1 - (t - 0.85) * 1.5), 16), hd)
    add(lathe('collar', [(0.0, 0.055), (0.0145, 0.055), (0.0145, 0.075), (0.008, 0.08), (0.0, 0.08)], 16), fr, 2)
    return [finish(out, 'SM_Umbrella', [cm, fr, hd], angle=45.0, weighted=False)]


# ================================================================== build

BUILDERS = {'couch': build_couch, 'table': build_table, 'chair': build_armchair, 'tv': build_tvstand, 'rug': build_rug, 'lamp': build_lamp,
            'radiator': build_radiator, 'shelf': build_shelf, 'plant': build_plant, 'kitchen': build_kitchenette, 'window': build_window,
            'door': build_door, 'umbrella': build_umbrella}


def build():
    core.reset()
    os.makedirs(SCRATCH, exist_ok=True)
    objs = []
    for key, fn in BUILDERS.items():
        if _ONLY and key not in _ONLY:
            continue
        t0 = time.time()
        made = fn()
        objs += made
        print(f'[dee_room] built {key} ({", ".join(o.name for o in made)}) in {time.time() - t0:.1f}s')
    for o in objs:
        o.data.name = o.name
    ALL[:] = objs
    return objs


def bake_parts(objs):
    bake = []
    for o in objs:
        if o.name in UNBAKED:
            o.hide_render = True
        else:
            bake.append(o)
    return bake


def assemble(objs):
    """After the bake: two-sided slots (cloth and leaves), and the 'pat' UVs (only the bake needed them) dropped."""
    for o in objs:
        names = SLOTS.get(o.name, [])
        for i, slot in enumerate(o.material_slots):
            if i < len(names) and names[i] in TWO_SIDED:
                slot.material.use_backface_culling = False
        pat = o.data.uv_layers.get('pat')
        if pat is not None:
            o.data.uv_layers.remove(pat)
    for o in ALL:
        o.hide_render = False
        pat = o.data.uv_layers.get('pat')
        if pat is not None and o.name in UNBAKED:
            o.data.uv_layers.remove(pat)
    return list(ALL)


# ================================================================== review

def _which():
    v = os.environ.get('DR_REVIEW', 'room,props')
    return {s.strip() for s in v.split(',') if s.strip()}


def _rv_mat(name, hex_color, rough=0.5, metal=0.0, emit=None, strength=0.0):
    m = core.Mat(name)
    m.set('Base Color', H(hex_color))
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    if emit is not None:
        m.set('Emission Color', H(emit))
        m.set('Emission Strength', strength)
    return m


def _light(name, kind_, loc, energy, color, size=0.1, target=None, spot=None):
    data = bpy.data.lights.new(name, kind_)
    data.energy = energy
    data.color = color
    if kind_ in ('POINT', 'SPOT'):
        data.shadow_soft_size = size
    if kind_ == 'AREA':
        data.shape = 'RECTANGLE'
        data.size, data.size_y = size if isinstance(size, tuple) else (size, size)
    if kind_ == 'SPOT' and spot:
        data.spot_size = math.radians(spot)
        data.spot_blend = 0.6
    obj = core.link(bpy.data.objects.new(name, data))
    obj.location = loc
    if target is not None:
        d = Vector(target) - Vector(loc)
        obj.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    return obj


def _cam(loc, target, lens=35.0):
    sc = bpy.context.scene
    if sc.camera is None or sc.camera.name != 'dr_cam':
        data = bpy.data.cameras.new('dr_cam')
        cam = core.link(bpy.data.objects.new('dr_cam', data))
        sc.camera = cam
    cam = sc.camera
    cam.data.lens = lens
    cam.data.clip_start = 0.02
    cam.data.clip_end = 200.0
    cam.location = loc
    d = Vector(target) - Vector(loc)
    cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    return cam


def _shot(path, w, h, samples=64):
    return review._render(path, w, h, samples)


def compose_grid(name, paths, cols):
    imgs = [review._pixels(p) for p in paths]
    h, w = imgs[0].shape[:2]
    rows = (len(imgs) + cols - 1) // cols
    gap = 6
    W = cols * w + (cols - 1) * gap
    Hh = rows * h + (rows - 1) * gap
    out = np.zeros((Hh, W, 4), np.float32)
    out[:, :, :3] = 0.05
    out[:, :, 3] = 1.0
    for k, im in enumerate(imgs):
        r, c = k // cols, k % cols
        y0 = Hh - (r + 1) * h - r * gap
        x0 = c * (w + gap)
        out[y0:y0 + h, x0:x0 + w] = im
    os.makedirs(core.REVIEW_DIR, exist_ok=True)
    path = os.path.join(core.REVIEW_DIR, f'{name}.jpg')
    img = bpy.data.images.new(f'rv_{name}', W, Hh, alpha=False)
    img.pixels.foreach_set(out.ravel())
    img.filepath_raw = path
    img.file_format = 'JPEG'
    img.save(quality=90)
    bpy.data.images.remove(img)
    print(f'[dee_room] review sheet {os.path.relpath(path, core.ROOT)}')
    return path


# Close looks per mesh: (mesh, [(label, azimuth (0 = from the front, -Y), elevation, distance factor, target offset)])
PROP_SHOTS = {
    'SM_DR_Couch': [('front', -25, 14, 1.25, (0, 0, 0.42)), ('close', 30, 28, 0.62, (-0.35, 0.0, 0.6)), ('end', 55, 22, 0.6, (0.62, -0.1, 0.55)),
                    ('back', 160, 25, 1.2, (0, 0, 0.5))],
    'SM_DR_CoffeeTable': [('front', -25, 24, 1.3, (0, 0, 0.28)), ('top', 15, 58, 0.55, (-0.33, 0.0, 0.45)), ('ashtray', 35, 48, 0.22, (0.40, 0.085, 0.46)),
                          ('shelf', -12, 14, 0.75, (0.0, 0.0, 0.14))],
    'SM_DR_Armchair': [('front', -30, 15, 1.25, (0, 0, 0.5)), ('cardigan', -70, 22, 0.75, (-0.4, -0.1, 0.5)), ('seat', 20, 40, 0.7, (0.05, -0.05, 0.6)),
                       ('back', 140, 18, 1.2, (0, 0, 0.55))],
    'SM_DR_TVStand': [('front', -22, 10, 1.2, (0, 0, 0.6)), ('screen', 8, 5, 0.6, (0, -0.2, 0.86)), ('bays', -10, 8, 0.55, (0.0, -0.2, 0.28)),
                      ('top', 30, 35, 0.6, (0.05, -0.05, 1.12))],
    'SM_DR_Rug': [('top', 0, 62, 0.85, (0, 0, 0)), ('corner', 40, 25, 0.3, (1.0, -0.68, 0.03)), ('fringe', -70, 22, 0.25, (-1.22, 0.2, 0.0)),
                  ('path', -15, 28, 0.55, (0.2, -0.2, 0.0))],
    'SM_DR_FloorLamp': [('front', -20, 8, 1.0, (0, 0, 0.85)), ('shade', 20, 12, 0.36, (0, 0, 1.38)), ('under', -30, -25, 0.3, (0, 0, 1.33)),
                        ('base', 30, 30, 0.35, (0, 0.15, 0.05))],
    'SM_DR_Radiator': [('front', -20, 15, 1.2, (0, 0, 0.35)), ('close', 30, 25, 0.45, (0.2, 0, 0.62)), ('valve', -60, 15, 0.38, (-0.5, 0, 0.12)),
                       ('top', 10, 60, 0.5, (0.03, 0, 0.68))],
    'SM_DR_Shelf': [('front', -15, 8, 0.95, (0, 0, 0.95)), ('mementos', -10, 6, 0.3, (0, -0.05, 0.88)), ('plaque', 12, 8, 0.27, (-0.06, 0.0, 1.22)),
                    ('top', -25, 22, 0.35, (0.15, -0.1, 1.66))],
    'SM_DR_Plant': [('front', -20, 12, 1.2, (0, 0, 0.42)), ('close', 30, 40, 0.65, (0, 0, 0.4))],
    'SM_DR_Kitchenette': [('front', -20, 15, 1.0, (0, 0, 0.8)), ('top', 15, 45, 0.5, (0.05, -0.02, 0.98)), ('calendar', -4, 4, 0.3, (0.12, 0.3, 1.32)),
                          ('hotplate', 35, 30, 0.32, (0.35, -0.1, 1.0))],
    'SM_DR_Window': [('front', -15, 5, 0.95, (0, 0, 1.45)), ('sill', -25, 25, 0.28, (0.15, 0.0, 0.9)), ('lock', 10, 15, 0.22, (0, 0.06, 1.62))],
    'SM_DR_Door': [('front', -15, 5, 1.0, (0, 0, 1.05)), ('hardware', -25, 8, 0.3, (-0.42, 0, 1.2)), ('foot', 20, 22, 0.35, (0, 0, 0.2))],
    'SM_Umbrella': [('side', -20, 8, 0.75, (0, 0, 0.45)), ('low', 40, -12, 0.6, (0, 0, 0.55)), ('top', 10, 55, 0.8, (0, 0, 0.8)),
                    ('grip', -60, 10, 0.18, (0.03, 0, -0.02))],
}


def _prop_review(by, tmp):
    """Each mesh alone under a warm lamp and a cool fill (art/review/dee_room_props_N.jpg)."""
    sc = bpy.context.scene
    world = sc.world
    world.color = (0.012, 0.011, 0.01)
    floor = core.link(bpy.data.objects.new('rv_floor', bpy.data.meshes.new('rv_floor')))
    floor.data.from_pydata([(-20, -20, 0), (20, -20, 0), (20, 20, 0), (-20, 20, 0)], [], [(0, 1, 2, 3)])
    core.assign(floor, _rv_mat('rv_floor_m', 0x3a2c22, 0.55))
    tmp.append(floor)
    paths = []
    for name, shots in PROP_SHOTS.items():
        if name not in by:
            continue
        show = [by[name]] + [by[n] for n in by if n.startswith(name + '_')]
        for o in by.values():
            o.hide_render = o not in show
        lo, hi = core.bounds([by[name]])
        floor.location.z = min(lo.z, 0.0) - 0.0005
        size = max((hi - lo).length, 0.3)
        c = (lo + hi) / 2
        key = _light('rv_key', 'AREA', (c.x - size * 0.9, c.y - size * 1.2, c.z + size * 1.1), 95.0 * size * size, (1.0, 0.8, 0.6), size * 0.6, c)
        fill = _light('rv_fill', 'AREA', (c.x + size * 1.4, c.y - size * 0.6, c.z + size * 0.5), 22.0 * size * size, (0.6, 0.72, 1.0), size, c)
        rim = _light('rv_rim', 'AREA', (c.x + size * 0.3, c.y + size * 1.4, c.z + size * 1.0), 60.0 * size * size, (1.0, 0.9, 0.8), size * 0.5, c)
        for label, az, el, dist, tgt in shots:
            t = Vector(tgt) + Vector((0, 0, 0)) if tgt is not None else c
            a, e = math.radians(az), math.radians(el)
            camp = t + Vector((math.sin(a) * math.cos(e), -math.cos(a) * math.cos(e), math.sin(e))) * size * dist * 1.4
            _cam(camp, t, 40.0)
            paths.append(_shot(os.path.join(SCRATCH, f'p_{name}_{label}.png'), 800, 800, 64))
        for l in (key, fill, rim):
            bpy.data.objects.remove(l)
    for o in by.values():
        o.hide_render = False
    floor.hide_render = True
    out = []
    for i in range(0, len(paths), 12):
        out.append(compose_grid(f'dee_room_props_{i // 12 + 1}', paths[i:i + 12], 4))
    return out


# Where each mesh stands in the review's room (x, y, yaw degrees): the couch against the back wall under nothing, the
# window and radiator to its left, the TV across from it, the kitchenette on the left wall, the door in the front wall.
ROOM = {'SM_DR_Couch': (0.15, 1.43, 0.0), 'SM_DR_Rug': (0.1, 0.72, 0.0), 'SM_DR_CoffeeTable': (0.15, 0.7, 0.0), 'SM_DR_Armchair': (-1.5, 0.3, 33.0),
        'SM_DR_TVStand': (0.1, -1.66, 180.0), 'SM_DR_FloorLamp': (1.5, 1.6, 0.0), 'SM_DR_Radiator': (-1.6, 1.76, 0.0), 'SM_DR_Window': (-1.6, 1.9, 0.0),
        'SM_DR_Plant': (-0.82, 1.62, 0.0), 'SM_DR_Shelf': (2.33, 0.45, -90.0), 'SM_DR_Kitchenette': (-2.19, -0.95, 90.0), 'SM_DR_Door': (1.55, -1.9, 180.0)}
ROOM_SHOTS = [('room_a', (2.1, -1.45, 1.62), (-0.7, 1.0, 0.75), 22.0), ('room_b', (0.25, 1.05, 1.1), (0.0, -1.9, 0.85), 24.0),
              ('room_c', (0.95, -0.25, 1.62), (-0.05, 1.25, 0.5), 26.0), ('room_d', (1.7, 0.3, 1.5), (-2.3, -0.4, 0.95), 22.0),
              ('room_e', (-0.4, -0.6, 1.55), (2.4, 0.7, 1.0), 26.0), ('room_f', (0.6, 0.15, 1.2), (-1.6, 1.75, 0.75), 28.0)]


def _room_review(by, tmp):
    """Every prop in a small lamp-lit room at night (art/review/dee_room_room.jpg): the floor lamp and the TV's glow,
    cool light from the street through the window."""
    sc = bpy.context.scene
    placed = {}
    for name, (x, y, yaw) in ROOM.items():
        for n, o in by.items():
            if n == name or n.startswith(name + '_'):
                o.location = (x, y, 0.0)
                o.rotation_mode = 'XYZ'
                o.rotation_euler = (0.0, 0.0, math.radians(yaw))
                placed[n] = o
    for n, o in by.items():
        o.hide_render = n not in placed or n == 'SM_DR_Window_Glass'
    bpy.context.view_layer.update()
    floor_m = core.Mat('rv_planks')
    tc, sep = object_coords(floor_m)
    plank = floor_m.math('FLOOR', floor_m.math('DIVIDE', sep.outputs['Y'], 0.085))
    wn = floor_m.node('ShaderNodeTexWhiteNoise')
    wn.noise_dimensions = '1D'
    floor_m.link(plank, wn.inputs['W'])
    grain = noise(floor_m, scaled(floor_m, sep, 3.0, 120.0, 3.0), detail=4.0)
    fcol = floor_m.mix(wn.outputs['Value'], H(0x3a2416), H(0x5a3a22))
    fcol = floor_m.mix(floor_m.math('MULTIPLY', grain, 0.5), fcol, H(0x2a180c))
    seam = smooth_less(floor_m, floor_m.math('ABSOLUTE', floor_m.math('SUBTRACT', floor_m.math('FRACT', floor_m.math('DIVIDE', sep.outputs['Y'], 0.085)), 0.5)), 0.49, 0.004)
    fcol = floor_m.mix(floor_m.math('SUBTRACT', 1.0, seam), fcol, H(0x120a06))
    floor_m.set('Base Color', fcol)
    floor_m.set('Roughness', 0.42)
    wall_m = core.Mat('rv_wallpaper')
    tc2, sep2 = object_coords(wall_m)
    stripe = smooth_less(wall_m, wall_m.math('ABSOLUTE', wall_m.math('SUBTRACT', wall_m.math('FRACT', wall_m.math('DIVIDE', wall_m.math('ADD', sep2.outputs['X'], sep2.outputs['Y']), 0.11)), 0.5)), 0.12, 0.02)
    wcol = wall_m.mix(wall_m.math('MULTIPLY', stripe, 0.35), H(0xbfae86), H(0xa8946a))
    wcol = wall_m.mix(wall_m.math('MULTIPLY', ramp(wall_m, noise(wall_m, tc2.outputs['Object'], scale=1.5, detail=4.0), 0.55, 0.75), 0.3), wcol, H(0x8a7650))
    wall_m.set('Base Color', wcol)
    wall_m.set('Roughness', 0.8)
    ceil_m = _rv_mat('rv_ceiling', 0xd8d2c4, 0.9)
    base_m = _rv_mat('rv_base', 0x2a1a10, 0.45)
    pieces = [((-2.65, 2.65, -2.05, 2.05, -0.02, 0.0), floor_m), ((-2.65, 2.65, -2.05, 2.05, 2.7, 2.72), ceil_m),
              ((-2.65, -2.5, -2.05, 2.05, 0.0, 2.7), wall_m), ((2.5, 2.65, -2.05, 2.05, 0.0, 2.7), wall_m)]
    wx0, wx1 = -1.6 - 0.43, -1.6 + 0.43
    pieces += [((-2.65, wx0, 1.9, 2.05, 0.0, 2.7), wall_m), ((wx1, 2.65, 1.9, 2.05, 0.0, 2.7), wall_m),
               ((wx0, wx1, 1.9, 2.05, 0.0, 0.826), wall_m), ((wx0, wx1, 1.9, 2.05, 2.3, 2.7), wall_m)]
    dx0, dx1 = 1.55 - 0.4575, 1.55 + 0.4575
    pieces += [((-2.65, dx0, -2.05, -1.9, 0.0, 2.7), wall_m), ((dx1, 2.65, -2.05, -1.9, 0.0, 2.7), wall_m), ((dx0, dx1, -2.05, -1.9, 2.035, 2.7), wall_m)]
    pieces += [((-2.5, 2.5, 1.888, 1.9, 0.0, 0.11), base_m), ((-2.5, -2.488, -1.9, 1.9, 0.0, 0.11), base_m), ((2.488, 2.5, -1.9, 1.9, 0.0, 0.11), base_m),
               ((-2.5, dx0 - 0.09, -1.9, -1.888, 0.0, 0.11), base_m)]
    for (x0, x1, y0, y1, z0, z1), mat in pieces:
        b = box(x0, x1, y0, y1, z0, z1)
        core.assign(b, mat)
        tmp.append(b)
    # the street outside the window and the door: dark
    out_m = _rv_mat('rv_outside', 0x05060a, 1.0)
    for (x0, x1, y0, y1, z0, z1) in ((wx0, wx1, 2.3, 2.32, 0.7, 2.4), (dx0, dx1, -2.4, -2.38, 0.0, 2.1)):
        b = box(x0, x1, y0, y1, z0, z1)
        core.assign(b, out_m)
        tmp.append(b)
    lamp = by.get('SM_DR_FloorLamp')
    lights = []
    if lamp is not None:
        bulb = lamp.matrix_world @ Vector(L_BULB)
        lights.append(_light('rv_bulb', 'POINT', tuple(bulb), 55.0, (1.0, 0.7, 0.42), 0.03))
    lights.append(_light('rv_tv', 'AREA', (0.1, -1.38, 0.88), 18.0, (0.5, 0.62, 1.0), (0.5, 0.38), (0.1, 1.0, 0.85)))
    lights.append(_light('rv_street', 'AREA', (-1.6, 2.6, 2.4), 260.0, (0.62, 0.68, 1.0), (1.2, 1.6), (-1.6, 0.5, 0.4)))
    lights.append(_light('rv_bounce', 'AREA', (0.0, 0.0, 2.6), 14.0, (1.0, 0.8, 0.6), (3.0, 3.0), (0.0, 0.0, 0.0)))
    tmp += lights
    sc.world.color = (0.002, 0.002, 0.003)
    sc.view_settings.exposure = 0.6
    paths = []
    for label, cp, tgt, lens in ROOM_SHOTS:
        _cam(cp, tgt, lens)
        paths.append(_shot(os.path.join(SCRATCH, f'{label}.png'), 1280, 720, 128))
    sc.view_settings.exposure = 0.0
    compose_grid('dee_room_room', paths, 3)
    for o in by.values():
        o.location = (0.0, 0.0, 0.0)
        o.rotation_euler = (0.0, 0.0, 0.0)
        o.hide_render = False
    for o in list(tmp):
        try:
            bpy.data.objects.remove(o)
        except ReferenceError:
            pass
    tmp.clear()
    bpy.context.view_layer.update()


def pose_for_review(objs):
    global REVIEW_VIEWS
    by = {o.name: o for o in objs}
    for o in objs:
        o.hide_render = False
        if o.name.endswith('_Glow'):
            for s in o.material_slots:
                m = s.material
                bs = next((n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
                if bs is not None:
                    bs.inputs['Emission Color'].default_value = H(0xffc890) if 'Lamp' in o.name else H(0x5a7ab0)
                    bs.inputs['Emission Strength'].default_value = 6.0 if 'Lamp' in o.name else 0.3
    tmp = []
    try:
        if 'room' in _which():
            _room_review(by, tmp)
        if 'props' in _which():
            _prop_review(by, tmp)
    except Exception:
        traceback.print_exc()
    finally:
        for o in tmp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        sc = bpy.context.scene
        if sc.camera is not None and sc.camera.name == 'dr_cam':
            bpy.data.objects.remove(sc.camera)
    # the standard sheet: two rows, the furniture in front, the rest behind; glows and glass stay with their meshes
    rows = [['SM_DR_Couch', 'SM_DR_CoffeeTable', 'SM_DR_Armchair', 'SM_DR_TVStand', 'SM_DR_Shelf', 'SM_DR_Kitchenette'],
            ['SM_DR_Rug', 'SM_DR_FloorLamp', 'SM_DR_Radiator', 'SM_DR_Plant', 'SM_DR_Window', 'SM_DR_Door', 'SM_Umbrella']]
    y = 0.0
    for row in rows:
        names = [n for n in row if n in by]
        if not names:
            continue
        ext = [core.bounds([by[n]]) for n in names]
        width = sum(hi.x - lo.x for lo, hi in ext) + 0.5 * (len(names) - 1)
        depth = max(hi.y - lo.y for lo, hi in ext)
        x = -width / 2
        for n, (lo, hi) in zip(names, ext):
            loc = Vector((x - lo.x, y - lo.y, -min(lo.z, 0.0)))
            for m_, o in by.items():
                if m_ == n or m_.startswith(n + '_'):
                    o.location = loc
            x += (hi.x - lo.x) + 0.5
        y += depth + 0.8
    bpy.context.view_layer.update()
    lo, hi = core.bounds(objs)
    cc = by.get('SM_DR_Couch')
    tgt = tuple(cc.location + Vector((0.0, 0.45, 0.5))) if cc is not None else ((lo.x + hi.x) / 2, 0.0, 0.5)
    REVIEW_VIEWS = [('front', -12, 18, 2.3), ('top', 12, 48, 2.1), ('couch', -28, 22, 0.28, tgt)]
