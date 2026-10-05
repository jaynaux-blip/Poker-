"""Real clothes for the street hero, fitted to the hero MetaHumans' own bodies and skinned to their skeleton.

Not a build.py asset (don't list it in ASSETS): it writes skeletal FBX files, not glb props. Run it with Blender 5.2:

    blender -b -P art/blender/assets/garments.py -- [A] [B] [--wardrobe DIR] [--no-bake] [--no-export] [--no-review]
        [--jeans-only] [--preview | --jacket-preview] [--look]

--wardrobe: where the hero bodies exported from Unreal are (default $SHORTSTACK_WARDROBE, else art/build/wardrobe):
<Hero>_SKM_MHC_<Hero>_BodyMesh.fbx and <Hero>_MHC_<Hero>_Outfits.fbx, as garments_setup.export_sources() writes them.

For each body type (A: HeroA1's body, B: HeroB1's), written to unreal/Art/Garments (FBX, Textures/, garments.json):
- SK_Garment_Jeans_<A|B>: straight-leg five-pocket jeans. Waistband with seven belt loops and a shank button, a
  button fly with its J-stitch, scoop front pockets with a coin pocket, patch back pockets, the back yoke and
  center-back seam, flat-felled inseam and rise, copper rivets, a leather patch, and a stacked hem breaking on the shoe.
  Indigo denim, faded at the thighs, knees and seat, whiskered at the hips, roped at the hem.
- SK_Garment_Jacket_<A|B>: a closed waist-length canvas bomber. Rib-knit stand collar, cuffs and hem band, an
  exposed brass center-front zip with its slider and pull, two slanted welt hand pockets, set-in sleeves, and the
  shell bloused over the ribs.
Plus the tiling weave normals Unreal lays over the bakes (T_Garment_Detail_Twill_N, _Canvas_N) and the review sheets
art/review/garments_<A|B>.jpg (four poses, front, side, back) and garments_<A|B>_detail.jpg.

How they're made: the MetaHuman body comes with the faces under its default tee and shorts removed, so the tee and
shorts stand in for the torso, pelvis and upper legs. Each garment is lofted from rings: the cloth's section at each
height (or along each arm) is the convex hull of the body's section there (cloth spans hollows) plus ease, smoothed
along the length so it can't follow a hollow either. The jeans are a pelvis tube that splits into two legs at the
crotch (the legs' first ring is half the pelvis's last ring and the crotch seam they share); the jacket is a torso
tube with an oval armhole cut in each side, a sleeve leaving each armhole along the torso's surface and bending down
the arm, the shoulders laid over the tee from the armholes' top up to a neck seam at the tee collar's inner edge (the
base of the neck), a rib stand collar rising from it, and rib bands. Folds are sculpted in (the stack at the hem,
behind the knee, gathers into the ribs, inside the elbow, the sleeves' twist, drape under the arms and down the back).
UVs are the pattern pieces at true scale, the garment's up along +V.
Weights come from the nearest point of the body and the tee/shorts (and the jeans, for the jacket's hem band),
limited to the bones the garment should follow and smoothed over the garment. The armature is the body's own (every
bone, its names and bind pose), so Unreal drives the garment with the body as its leader pose.

Material slots. Jeans: 0 denim, 1 metal (button). Jacket: 0 shell (bakes light neutral grey: the game tints it with
the creator's outfit color), 1 rib knit (light grey: tinted a darker shade of the same color), 2 the zip (brass teeth,
tape, slider and pull).

Coordinates (meters, Blender): the body's own, facing -Y, its left toward +X, Z up, the floor at Z = 0. The FBX
carries them in centimeters under the armature "root" (the root bone), as Unreal exported the body.
"""
import math
import os
import sys
import time

import bpy  # noqa: I001
import bmesh
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from artkit import core, texmaps  # noqa: E402

GARMENT_DIR = os.path.join(core.ROOT, 'unreal', 'Art', 'Garments')
WARDROBE_DEFAULT = os.environ.get('SHORTSTACK_WARDROBE', os.path.join(core.BUILD_DIR, 'wardrobe'))
# The hero each body type's garments are fitted to (hero_cast.py: HeroA0..2, HeroB0..2 share a body type).
HEROES = {'A': 'HeroA1', 'B': 'HeroB1'}
TEXTURE_SIZE = 2048


# ------------------------------------------------------------------ source bodies

class Hero:
    """A hero's body, armature and default outfit, in world meters (bind pose)."""

    def __init__(self, key, wardrobe):
        self.key = key
        self.name = HEROES[key]
        body_path = os.path.join(wardrobe, f'{self.name}_SKM_MHC_{self.name}_BodyMesh.fbx')
        outfit_path = os.path.join(wardrobe, f'{self.name}_MHC_{self.name}_Outfits.fbx')
        before = set(bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=body_path)
        new = [o for o in bpy.data.objects if o not in before]
        self.arm = next(o for o in new if o.type == 'ARMATURE')
        self.body = next(o for o in new if o.type == 'MESH')
        holder = self.arm.parent
        # The armature on its own at the top (Unreal reads a parent node as a bone): keep its world transform.
        mw = self.arm.matrix_world.copy()
        self.arm.parent = None
        self.arm.matrix_world = mw
        if holder is not None and holder.type == 'EMPTY':
            bpy.data.objects.remove(holder)
        self.arm.name = 'root'
        self.arm.data.name = 'root'
        self.body.name = f'Body_{key}'
        # The FBX brings no textures: skin for the reviews (the body itself isn't exported).
        skin = core.Mat(f'review_skin_{key}')
        skin.set('Base Color', core.hex_linear(0x8f6248 if key == 'A' else 0x9a6a4f))
        skin.set('Roughness', 0.5)
        skin.set('Subsurface Weight', 0.25)
        skin.set('Subsurface Radius', (0.9, 0.45, 0.25))
        skin.set('Subsurface Scale', 0.01)
        self.body.data.materials.clear()
        self.body.data.materials.append(skin.m)
        before = set(bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=outfit_path)
        new = [o for o in bpy.data.objects if o not in before]
        self.outfit = next(o for o in new if o.type == 'MESH')
        for o in new:
            if o is not self.outfit:
                bpy.data.objects.remove(o)
        bpy.context.view_layer.update()
        self.outfit.parent = self.arm
        self.outfit.matrix_parent_inverse = Matrix.Identity(4)
        self.outfit.matrix_world = self.body.matrix_world.copy()
        for m in self.outfit.modifiers:
            if m.type == 'ARMATURE':
                m.object = self.arm
        self.outfit.name = f'Outfit_{key}'
        bpy.context.view_layer.update()
        self.bones = {b.name: np.array(self.arm.matrix_world @ b.head_local) for b in self.arm.data.bones}
        self.bone_names = [b.name for b in self.arm.data.bones]
        # Triangles in world meters.
        self.bV, self.bF = tri_mesh(self.body)
        self.oV, self.oF, self.oMat, self.oIsl = tri_mesh(self.outfit, materials=True, islands=True)
        names = [m.name for m in self.outfit.data.materials]
        shirt = {i for i, n in enumerate(names) if 'Shirt' in n}
        self.o_shirt = np.isin(self.oMat, list(shirt))
        self.floor = float(min(0.0, max(-0.01, self.bV[:, 2].min())))

    def bone(self, name):
        return self.bones[name]


def tri_mesh(obj, materials=False, islands=False):
    """World-space vertices and triangles of a mesh (its rest shape); per triangle its material and island."""
    me = obj.data
    mw = np.array(obj.matrix_world)
    co = np.empty(len(me.vertices) * 3, dtype=np.float64)
    me.vertices.foreach_get('co', co)
    co = co.reshape(-1, 3) @ mw[:3, :3].T + mw[:3, 3]
    me.calc_loop_triangles()
    tris = np.empty(len(me.loop_triangles) * 3, dtype=np.int64)
    me.loop_triangles.foreach_get('vertices', tris)
    tris = tris.reshape(-1, 3)
    out = [co, tris]
    if materials:
        mi = np.empty(len(me.loop_triangles), dtype=np.int64)
        me.loop_triangles.foreach_get('material_index', mi)
        out.append(mi)
    if islands:
        out.append(vertex_islands(me)[tris[:, 0]])
    return tuple(out)


def vertex_islands(me):
    """Connected component index per vertex."""
    n = len(me.vertices)
    parent = np.arange(n)

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    ev = np.empty(len(me.edges) * 2, dtype=np.int64)
    me.edges.foreach_get('vertices', ev)
    for a, b in ev.reshape(-1, 2):
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[ra] = rb
    return np.array([find(i) for i in range(n)])


# ------------------------------------------------------------------ sections

def section_points(V, F, origin, normal, mask=None):
    """Points where triangles cross a plane (two per crossing triangle), world space."""
    o = np.asarray(origin, float)
    n = np.asarray(normal, float)
    n = n / np.linalg.norm(n)
    if mask is not None:
        F = F[mask]
    d = (V - o) @ n
    dt = d[F]
    cross = (dt.min(1) < 0.0) & (dt.max(1) > 0.0)
    F = F[cross]
    dt = dt[cross]
    pts = []
    for a, b in ((0, 1), (1, 2), (2, 0)):
        da, db = dt[:, a], dt[:, b]
        hit = (da < 0.0) != (db < 0.0)
        t = da[hit] / (da[hit] - db[hit])
        pa, pb = V[F[hit, a]], V[F[hit, b]]
        pts.append(pa + (pb - pa) * t[:, None])
    return np.concatenate(pts) if pts else np.zeros((0, 3))


def hull2d(P):
    """Convex hull (counter-clockwise) of 2D points (monotone chain)."""
    P = np.unique(np.round(np.asarray(P, float), 7), axis=0)
    if len(P) < 3:
        return P
    P = P[np.lexsort((P[:, 1], P[:, 0]))]

    def half(pts):
        h = []
        for p in pts:
            while len(h) >= 2 and (h[-1][0] - h[-2][0]) * (p[1] - h[-2][1]) - (h[-1][1] - h[-2][1]) * (p[0] - h[-2][0]) <= 0:
                h.pop()
            h.append(p)
        return h

    lower = half(P)
    upper = half(P[::-1])
    return np.array(lower[:-1] + upper[:-1])


def radial(hull, center, thetas):
    """Distance from center to the hull's boundary along each direction (radians, from +u counter-clockwise)."""
    c = np.asarray(center, float)
    A = hull - c
    B = np.roll(hull, -1, axis=0) - c
    out = np.zeros(len(thetas))
    for i, th in enumerate(thetas):
        d = np.array([math.cos(th), math.sin(th)])
        e = B - A
        den = d[0] * e[:, 1] - d[1] * e[:, 0]
        ok = np.abs(den) > 1e-12
        t = np.where(ok, (A[:, 0] * e[:, 1] - A[:, 1] * e[:, 0]) / np.where(ok, den, 1.0), -1.0)
        s = np.where(ok, (A[:, 0] * d[1] - A[:, 1] * d[0]) / np.where(ok, den, 1.0), -1.0)
        good = ok & (t > 0.0) & (s >= -1e-9) & (s <= 1.0 + 1e-9)
        out[i] = t[good].max() if good.any() else 0.0
    return out


def smooth1d(a, sigma, axis=0, wrap=False):
    """Gaussian smoothing (sigma in samples) along an axis; ends held (or wrapped)."""
    if sigma <= 0:
        return a
    r = int(math.ceil(sigma * 3))
    k = np.exp(-0.5 * (np.arange(-r, r + 1) / sigma) ** 2)
    k /= k.sum()
    a = np.moveaxis(np.asarray(a, float), axis, 0)
    pad = np.concatenate([a[-r:], a, a[:r]]) if wrap else np.concatenate([np.repeat(a[:1], r, 0), a, np.repeat(a[-1:], r, 0)])
    out = np.zeros_like(a)
    for i, w in enumerate(k):
        out += w * pad[i:i + len(a)]
    return np.moveaxis(out, 0, axis)




def span_fill(r, ds, slope):
    """Cloth can't dip into a hollow along its length: r[i] >= r[k] - slope * |i - k| * ds for every k (along axis 0)."""
    r = np.asarray(r, float)
    out = r.copy()
    idx = np.arange(len(r))
    for i in range(len(r)):
        d = np.abs(idx - i) * ds * slope
        out[i] = np.max(r - (d[:, None] if r.ndim > 1 else d), axis=0)
    return out


def smoothstep(e0, e1, x):
    t = np.clip((np.asarray(x, float) - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def poly_centroid(P):
    x, y = P[:, 0], P[:, 1]
    x1, y1 = np.roll(x, -1), np.roll(y, -1)
    cr = x * y1 - x1 * y
    a = cr.sum() / 2.0
    if abs(a) < 1e-10:
        return P.mean(0)
    return np.array([((x + x1) * cr).sum(), ((y + y1) * cr).sum()]) / (6.0 * a)


def poly_centroid_area(P):
    x, y = P[:, 0], P[:, 1]
    return float((x * np.roll(y, -1) - np.roll(x, -1) * y).sum() / 2.0)


def poly_perimeter(P):
    return float(np.linalg.norm(np.roll(P, -1, 0) - P, axis=1).sum())


def z_sections(V, F, zs):
    """Crossing points of horizontal planes at each z."""
    tz = V[F][:, :, 2]
    lo, hi = tz.min(1), tz.max(1)
    out = []
    for z in zs:
        m = (lo < z) & (hi > z)
        out.append(section_points(V, F[m], (0.0, 0.0, z), (0.0, 0.0, 1.0)) if m.any() else np.zeros((0, 3)))
    return out


NTH = 360  # angular samples of a table


class Table:
    """A tube of cloth: its radius around a center line, by height and angle (radians, from +X toward +Y)."""

    def __init__(self, zs, C, R):
        self.zs = np.asarray(zs, float)
        self.C = np.asarray(C, float)
        self.R = np.asarray(R, float)
        self.dz = self.zs[1] - self.zs[0]
        self.n = self.R.shape[1]

    def _zi(self, z):
        f = np.clip((np.asarray(z, float) - self.zs[0]) / self.dz, 0.0, len(self.zs) - 1.000001)
        i = np.floor(f).astype(int)
        return i, f - i

    def center(self, z):
        i, t = self._zi(z)
        return self.C[i] * (1.0 - t)[..., None] + self.C[i + 1] * t[..., None]

    def radius(self, z, th):
        i, t = self._zi(z)
        a = (np.asarray(th, float) % (2 * math.pi)) / (2 * math.pi) * self.n
        j = np.floor(a).astype(int) % self.n
        u = a - np.floor(a)
        j1 = (j + 1) % self.n
        r0 = self.R[i, j] * (1 - u) + self.R[i, j1] * u
        r1 = self.R[i + 1, j] * (1 - u) + self.R[i + 1, j1] * u
        return r0 * (1 - t) + r1 * t

    def point(self, z, th, mirror=False):
        z = np.asarray(z, float)
        th = np.asarray(th, float)
        c = self.center(z)
        r = self.radius(z, th)
        x = c[..., 0] + r * np.cos(th)
        y = c[..., 1] + r * np.sin(th)
        return np.stack([-x if mirror else x, y, z * np.ones_like(x)], -1)


THS = np.linspace(0.0, 2 * math.pi, NTH, endpoint=False)


def fill_levels(items):
    """Replaces missing (None) entries with the nearest present one."""
    have = [i for i, x in enumerate(items) if x is not None]
    return [items[min(have, key=lambda k: abs(k - i))] if x is None else x for i, x in enumerate(items)]


# ------------------------------------------------------------------ shoes (as the game fits them)

def put_on_shoes(h):
    """The street's sneakers on the hero's feet, placed as AShortStackCharacter::BuildShoes places them (Shoe::ModelFit),
    carried by the foot bones. The jeans' hems rest on them."""
    shoes = []
    for side, s in ((0, 'l'), (1, 'r')):
        before = set(bpy.data.objects)
        bpy.ops.import_scene.gltf(filepath=os.path.join(core.ROOT, 'unreal', 'Art', 'Meshes', f'SM_Sneaker_{s.upper()}.glb'))
        new = [o for o in bpy.data.objects if o not in before]
        shoe = next(o for o in new if o.type == 'MESH')
        for o in new:
            if o is not shoe:
                bpy.data.objects.remove(o)
        foot, ball = h.bone(f'foot_{s}'), h.bone(f'ball_{s}')
        d = (ball - foot)[:2]
        length = float(np.linalg.norm(d))
        ahead = d / length
        size = min(max(length, 0.09), 0.19) / 0.142
        tall = min(max((foot[2] - h.floor) / (0.085 * size), 0.92), 1.12)
        at = Vector((foot[0], foot[1], h.floor - 0.003 + 0.085 * size * tall))
        yaw = math.atan2(ahead[1], ahead[0]) - math.pi / 2
        world = Matrix.Translation(at) @ Matrix.Rotation(yaw, 4, 'Z') @ Matrix.Diagonal((size, size, size * tall, 1.0))
        shoe.parent = h.arm
        shoe.parent_type = 'BONE'
        shoe.parent_bone = f'foot_{s}'
        bpy.context.view_layer.update()
        shoe.matrix_world = world
        shoe.name = f'Shoe_{s.upper()}_{h.key}'
        shoes.append(shoe)
    bpy.context.view_layer.update()
    return shoes


# ------------------------------------------------------------------ mesh assembly

class Builder:
    """Vertices, quads, and per-face material slot and UV island, assembled into one mesh."""

    def __init__(self):
        self.V = []
        self.count = 0
        self.faces = []
        self.fmat = []
        self.fisl = []
        self.fold = []

    def verts(self, P, fold=None):
        P = np.asarray(P, float)
        shape = P.shape[:-1]
        P = P.reshape(-1, 3)
        idx = np.arange(self.count, self.count + len(P))
        self.V.append(P)
        self.fold.append(np.zeros(len(P)) if fold is None else np.asarray(fold, float).reshape(-1))
        self.count += len(P)
        return idx.reshape(shape)

    def face(self, vs, mat=0, isl=0):
        self.faces.append(tuple(int(v) for v in vs))
        self.fmat.append(mat)
        self.fisl.append(isl)

    def grid(self, G, wrap=False, mat=0, isl=0):
        """Quads between consecutive rows of an index grid (rows x cols); isl and mat may be functions of (row, col)."""
        rows, cols = G.shape
        for r in range(rows - 1):
            for c in range(cols if wrap else cols - 1):
                c1 = (c + 1) % cols
                q = (G[r, c], G[r, c1], G[r + 1, c1], G[r + 1, c])
                if len(set(q)) < 3:
                    continue
                if len(set(q)) == 3:
                    q = tuple(dict.fromkeys(q))
                self.face(q, mat(r, c) if callable(mat) else mat, isl(r, c) if callable(isl) else isl)

    def positions(self):
        return np.concatenate(self.V) if self.V else np.zeros((0, 3))

    def build(self, name):
        P = self.positions()
        me = bpy.data.meshes.new(name)
        me.from_pydata([tuple(p) for p in P], [], self.faces)
        me.update()
        obj = core.link(bpy.data.objects.new(name, me))
        mi = np.array(self.fmat, dtype=np.int64)
        me.polygons.foreach_set('material_index', mi)
        isl = me.attributes.new('island', 'INT', 'FACE')
        isl.data.foreach_set('value', np.array(self.fisl, dtype=np.int32))
        fold = me.attributes.new('fold', 'FLOAT', 'POINT')
        fold.data.foreach_set('value', np.concatenate(self.fold).astype(np.float32))
        return obj


def outward_normals(obj):
    """Recomputes face normals consistently, pointing away from the mesh's interior (the body)."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def vertex_normals(obj):
    me = obj.data
    me.update()
    n = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get('normal', n)
    return n.reshape(-1, 3)


def get_co(obj):
    co = np.empty(len(obj.data.vertices) * 3)
    obj.data.vertices.foreach_get('co', co)
    return co.reshape(-1, 3)


def set_co(obj, P):
    obj.data.vertices.foreach_set('co', np.asarray(P, float).ravel())
    obj.data.update()


def grid_normals(P, center):
    """Vertex normals of a (rows, cols, 3) ring grid whose columns wrap, pointing away from its axis (center(z) -> xy)."""
    dc = np.roll(P, -1, axis=1) - np.roll(P, 1, axis=1)
    dr = np.empty_like(P)
    dr[1:-1] = P[2:] - P[:-2]
    dr[0] = P[1] - P[0]
    dr[-1] = P[-1] - P[-2]
    n = np.cross(dr, dc)
    n /= np.linalg.norm(n, axis=-1, keepdims=True) + 1e-12
    out = P.copy()
    out[..., :2] -= center(P[..., 2])
    out[..., 2] = 0.0
    s = np.sign((n * out).sum(-1, keepdims=True))
    s[s == 0] = 1.0
    return n * s


# ------------------------------------------------------------------ jeans

JEANS_M = 80                                   # columns around the pelvis
JEANS_N = 56                                   # columns around each leg
JEANS_BRIDGE = JEANS_N - (JEANS_M // 2 + 1)    # crotch-seam vertices shared by the legs (15)
J_OUT, J_B, J_IN = 20, 40, 48                  # leg columns: outseam, center back (ring 0), inseam; 0 is center front
WAISTBAND = (0.0, 0.006, 0.013, 0.020, 0.027, 0.033, 0.038)  # rows below the waistband's top edge; the last is its seam
(J_FRONT_L, J_BACK_L, J_FRONT_R, J_BACK_R, J_WAISTBAND, J_WB_FACING, J_HEM_FACING_L, J_HEM_FACING_R,
 J_POCKET_L, J_POCKET_R, J_LOOP, J_BUTTON, J_WAISTBAND_R, J_WB_FACING_R, J_HEM_FACING_LB, J_HEM_FACING_RB) = range(16)
J_BANDS = (J_WAISTBAND, J_WAISTBAND_R)
J_HEMS = (J_HEM_FACING_L, J_HEM_FACING_R, J_HEM_FACING_LB, J_HEM_FACING_RB)
J_INSIDE = (J_WB_FACING, J_WB_FACING_R) + J_HEMS


class Fit:
    pass


def jeans_fit(h):
    f = Fit()
    shorts = h.oF[~h.o_shirt]
    sag = section_points(h.oV, shorts, (0.0, 0.0, 0.0), (1.0, 0.0, 0.0))
    f.z_crotch = float(sag[:, 2].min()) - 0.008
    f.z_split = f.z_crotch + 0.035
    top = float(h.oV[np.unique(shorts)][:, 2].max())
    f.z_wb_b = top - 0.004
    f.z_wb_f = f.z_wb_b - 0.028
    f.z_knee = float(h.bone('calf_l')[2])
    f.ankle_cut = float(h.bone('foot_l')[2]) + 0.035
    return f


def pelvis_table(h, f):
    """The jeans' seat and hips from the shorts (made symmetric), eased a little, snug at the waistband."""
    shorts = h.oF[~h.o_shirt]
    zs = np.arange(f.z_split - 0.03, f.z_wb_b + 0.015, 0.005)
    hulls = []
    for p in z_sections(h.oV, shorts, zs):
        if len(p) < 6:
            hulls.append(None)
            continue
        q = p[:, :2]
        hulls.append(hull2d(np.concatenate([q, q * [-1.0, 1.0]])))
    # Near the shorts' top edge (lower at the front) a section is only part of a ring: above the last whole one the
    # waistband goes straight up.
    area = np.array([abs(poly_centroid_area(H)) if H is not None else 0.0 for H in hulls])
    ref = np.median(area[(zs > f.z_split) & (zs < f.z_split + 0.15)])
    whole = [i for i in range(len(zs)) if area[i] > 0.93 * ref * (1.0 if zs[i] < f.z_split + 0.15 else 0.8)]
    top = max(i for i in whole if zs[i] < f.z_wb_f - 0.008)
    for i in range(top + 1, len(zs)):
        hulls[i] = hulls[top]
    hulls = fill_levels(hulls)
    yc = smooth1d(np.array([(H[:, 1].min() + H[:, 1].max()) / 2 for H in hulls]), 3)
    C = np.stack([np.zeros_like(yc), yc], 1)
    ease = np.interp(zs, [f.z_split, f.z_wb_f - 0.05, f.z_wb_f - 0.01], [0.006, 0.006, 0.0035])
    R = np.array([radial(H, c, THS) for H, c in zip(hulls, C)]) + ease[:, None]
    R = smooth1d(smooth1d(R, 1.5, 0), 2, 1, wrap=True)
    return Table(zs, C, R)


def shoe_tris(shoes, mirror_right=True):
    """The shoes' triangles in the left leg's frame (the right shoe mirrored onto the left)."""
    out = []
    for i, s in enumerate(shoes):
        V, F = tri_mesh(s)
        if i == 1 and mirror_right:
            V = V * [-1.0, 1.0, 1.0]
        out.append((V, F))
    return out


def leg_table(h, f, shoes):
    """A leg of the jeans (the left; the right is its mirror image): eased over the thigh, then straight from the knee
    to a hem that rests on the shoe. Returns the table and the hem's height by angle."""
    zs = np.arange(0.0, f.z_split + 0.03, 0.005)
    tz = h.bV[h.bF][:, :, 2]
    bF = h.bF[tz.min(1) > f.ankle_cut]
    shorts = h.oF[~h.o_shirt]
    hulls = []
    for pb, ps in zip(z_sections(h.bV, bF, zs), z_sections(h.oV, shorts, zs)):
        p = np.concatenate([pb, ps])[:, :2]
        q = np.concatenate([p[p[:, 0] > 0.0], p[p[:, 0] < 0.0] * [-1.0, 1.0]])
        hulls.append(hull2d(q) if len(q) >= 6 else None)
    valid = [i for i, H in enumerate(hulls) if H is not None]
    i_low = valid[0]
    craw = np.array([poly_centroid(H) if H is not None else (np.nan, np.nan) for H in hulls])
    craw[:i_low] = craw[i_low]
    for i in range(len(craw)):
        if np.isnan(craw[i, 0]):
            craw[i] = craw[i - 1]
    ik = int(np.argmin(np.abs(zs - f.z_knee)))
    c_knee = craw[ik - 2:ik + 3].mean(0)
    c_hem = craw[i_low] + [0.0, -0.006]
    straight = c_knee + (c_hem - c_knee) * np.clip((f.z_knee - zs) / (f.z_knee - 0.03), 0.0, 1.0)[:, None]
    w = smoothstep(f.z_knee - 0.03, f.z_knee + 0.05, zs)[:, None]
    C = smooth1d(w * craw + (1 - w) * straight, 4, 0)
    ease = np.interp(zs, [0.0, f.z_knee - 0.1, f.z_knee, f.z_knee + 0.15, f.z_split - 0.06, f.z_split],
                     [0.012, 0.012, 0.016, 0.013, 0.008, 0.006])
    Rh = np.array([radial(H, c, THS) if H is not None else np.zeros(NTH) for H, c in zip(hulls, C)])
    Rh = np.where(Rh > 0.0, Rh + ease[:, None], 0.0)
    # The straight leg below the knee: an ellipse that keeps the knee's width, narrowing a little to the hem.
    per = np.array([poly_perimeter(H) if H is not None else 0.0 for H in hulls])
    per_knee = per[np.abs(zs - f.z_knee) <= 0.0101].mean()
    calf = per[(zs > f.ankle_cut) & (zs < f.z_knee)].max()
    L_knee = per_knee + 2 * math.pi * 0.016
    L_hem = max(0.9 * L_knee, calf + 0.05, 0.37)
    L = np.interp(zs, [0.0, f.z_knee], [L_hem, L_knee]) * (1.0 - smoothstep(f.z_knee + 0.02, f.z_knee + 0.10, zs))
    q = 0.92
    a = L / (math.pi * (3 * (1 + q) - math.sqrt((3 + q) * (1 + 3 * q))))
    b = q * a
    den = np.sqrt((b[:, None] * np.cos(THS)) ** 2 + (a[:, None] * np.sin(THS)) ** 2)
    RE = np.where(den > 0, a[:, None] * b[:, None] / np.maximum(den, 1e-12), 0.0)
    R = np.maximum(Rh, RE)
    R = smooth1d(smooth1d(R, 3, 0), 3, 1, wrap=True)
    R = smooth1d(span_fill(R, 0.005, 0.45), 2, 0)
    tab = Table(zs, C, R)
    f.L_knee, f.L_hem = L_knee, L_hem
    # The hem rests on the shoe: above it where the shoe is under the hem (the instep), 2.8 cm off the floor elsewhere.
    trees = [BVHTree.FromPolygons([tuple(v) for v in V], [tuple(t) for t in F]) for V, F in shoe_tris(shoes)]
    ring = tab.point(0.05, THS)
    req = np.full(NTH, 0.028)
    for k, p in enumerate(ring):
        if math.sin(THS[k]) > 0.2:
            continue  # round the heel the hem stays low and clears the counter instead (below)
        for t in trees:
            hit = t.ray_cast(Vector((p[0], p[1], 0.4)), Vector((0.0, 0.0, -1.0)), 1.0)
            if hit[0] is not None:
                req[k] = max(req[k], hit[0].z + 0.007)
    hem = req.copy()
    for _ in range(10):
        hem = np.maximum(req, smooth1d(hem, 9, wrap=True))
    # Clear of the shoe all round wherever the leg comes down past it (the heel counter, the sides of the collar).
    need = np.zeros_like(R)
    lo_levels = np.where(zs < 0.14)[0]
    secs = [z_sections(V, F, zs[lo_levels]) for V, F in shoe_tris(shoes)]
    for n_i, i in enumerate(lo_levels):
        pts = np.concatenate([s[n_i] for s in secs])
        if len(pts) < 6:
            continue
        rs = radial(hull2d(pts[:, :2]), C[i], THS)
        on = (zs[i] >= hem - 0.004) & (rs > 0.0)
        need[i] = np.where(on, rs + 0.009, 0.0)
    R = np.maximum(R, need)
    R = np.maximum(smooth1d(R, 1.0, 0), need)
    # The legs meet at the middle (heavy thighs touch): neither crosses it.
    cth = np.cos(THS)[None, :]
    lim = np.where(cth < -1e-3, (C[:, :1] - 0.002) / np.maximum(-cth, 1e-3), np.inf)
    R = np.minimum(R, lim)
    return Table(zs, C, R), hem


def jeans_rows(f, hem_ref):
    """Normalized row positions down a leg (0 at the crotch ring, 1 at the hem): finer at the knee and in the stack."""
    zz = [f.z_split]
    while zz[-1] > hem_ref + 1e-5:
        z = zz[-1]
        if z - hem_ref < 0.2:
            ds = 0.005
        elif abs(z - f.z_knee) < 0.07:
            ds = 0.0065
        elif z > f.z_split - 0.04:
            ds = 0.006
        else:
            ds = 0.009
        zz.append(max(hem_ref, z - ds))
    if len(zz) > 2 and zz[-2] - zz[-1] < 0.002:
        del zz[-2]
    zz = np.array(zz)
    return (f.z_split - zz) / (f.z_split - hem_ref)


def leg_folds(f, Z, phi, hemj, side):
    """Sculpted folds on a leg, along the normal (m): the stack over the hem, the break on the shoe, behind the knee."""
    zr = Z - hemj
    front = np.clip(-np.sin(phi), 0.0, 1.0)
    rng = np.random.default_rng(11 + 7 * side)
    folds = []
    # The stack: broken, tilted ridges that wrap part way round, deepest just over the hem, mostly at the front and sides.
    h = 0.03 + rng.uniform(0.0, 0.006)
    while h < 0.2:
        k = 1.0 - smoothstep(0.03, 0.2, h)
        for _ in range(int(rng.integers(1, 3)) + (1 if h < 0.1 else 0)):
            folds.append((h + rng.uniform(-0.006, 0.006), rng.uniform(-math.pi, math.pi), rng.uniform(0.7, 1.6),
                          rng.uniform(-0.022, 0.022), (0.0018 + 0.0042 * k) * rng.uniform(0.75, 1.2), rng.uniform(0.0055, 0.008)))
        h += rng.uniform(0.026, 0.04)
    d = ridges(zr, phi, folds) * (0.55 + 0.45 * front) * smoothstep(0.01, 0.026, zr)
    # The break: where the front comes down onto the shoe.
    d += ridges(zr, phi, [(0.058, -math.pi / 2, 1.1, 0.012, 0.0055, 0.011)])
    # Behind the knee: a few soft creases.
    zk = Z - f.z_knee
    knee = [(-0.035 + 0.032 * i + rng.uniform(-0.005, 0.005), math.pi / 2 + rng.uniform(-0.3, 0.3), rng.uniform(0.7, 1.0),
             rng.uniform(-0.012, 0.012), 0.0024 * rng.uniform(0.8, 1.1), 0.007) for i in range(3)]
    d += ridges(zk, phi, knee)
    return d


def ridges(z, phi, folds):
    """Fold ridges along a tube: each (height, angle, half-width in angle, tilt per radian, depth, width), with a shallow
    trough either side."""
    d = np.zeros(np.broadcast(z, phi).shape)
    for h, pc, wid, tilt, amp, sig in folds:
        dp = (phi - pc + math.pi) % (2 * math.pi) - math.pi
        win = np.exp(-(dp / wid) ** 4)
        x = z - h - tilt * dp
        d += amp * win * (np.exp(-(x / sig) ** 2) - 0.3 * np.exp(-(x / (2.4 * sig)) ** 2))
    return d


def fly_ridge(f, P, psi, ztop, pel):
    """The fly: its shield doubles the cloth left of center front (the wearer's left, +X) down to a J at the bottom."""
    r = pel.radius(P[..., 2], psi)
    arc = (psi - 1.5 * math.pi) * r                 # along the waist from center front, + toward the wearer's left
    depth = ztop - P[..., 2]
    width = 0.034 * np.sqrt(np.clip(1.0 - ((depth - 0.13) / 0.045).clip(0.0, None) ** 2, 0.0, 1.0))
    band = smoothstep(-0.002, 0.003, arc) * (1.0 - smoothstep(width - 0.004, width + 0.002, arc)) * (depth < 0.178)
    groove = np.exp(-(arc / 0.0025) ** 2) * (depth < 0.172) * smoothstep(0.03, 0.045, depth)
    return 0.0013 * band - 0.0009 * groove


def build_jeans(h, shoes):
    f = jeans_fit(h)
    pel = pelvis_table(h, f)
    leg, hem = leg_table(h, f, shoes)
    M, N, NB = JEANS_M, JEANS_N, JEANS_BRIDGE
    psi = np.arange(M) * 2 * math.pi / M
    ztop = f.z_wb_f + (f.z_wb_b - f.z_wb_f) * (1.0 + np.sin(psi)) / 2.0
    rows = [ztop - d for d in WAISTBAND]
    seam = ztop - WAISTBAND[-1]
    n_body = max(4, int(round((seam.mean() - f.z_split) / 0.0095)))
    for u in np.linspace(0.0, 1.0, n_body + 1)[1:]:
        rows.append(seam + (f.z_split - seam) * u)
    Zp = np.array(rows)
    PSI = np.broadcast_to(psi, Zp.shape)
    Pp = pel.point(Zp, PSI)
    # The crotch seam: under the body from center back to center front, along the shorts' own seam.
    shorts = h.oF[~h.o_shirt]
    sag = section_points(h.oV, shorts, (0.0, 0.0, 0.0), (1.0, 0.0, 0.0))
    sag = sag[sag[:, 2] <= f.z_split + 0.04][:, 1:]
    Hs = hull2d(sag)
    kB, kF = M // 4, 3 * M // 4
    pB, pF = Pp[-1, kB], Pp[-1, kF]
    c = np.array([(pB[1] + pF[1]) / 2.0, f.z_split + 0.015])
    aB = math.atan2(pB[2] - c[1], pB[1] - c[0])
    aF = math.atan2(pF[2] - c[1], pF[1] - c[0])
    t = np.arange(1, NB + 1) / (NB + 1)
    al = aB + (aF - aB) * t
    rB = float(np.hypot(pB[1] - c[0], pB[2] - c[1]))
    rF = float(np.hypot(pF[1] - c[0], pF[2] - c[1]))
    eB, eF = radial(Hs, c, [aB, aF]) + 0.008
    rr = radial(Hs, c, al) + 0.008 + (rB - eB) * (1 - t) + (rF - eF) * t
    bridge_P = np.stack([np.zeros(NB), c[0] + rr * np.cos(al), c[1] + rr * np.sin(al)], 1)
    # The legs: ring 0 is half the pelvis's bottom ring and the crotch seam; rows run down to the hem.
    phiu = (np.arange(N) - J_OUT) * 2 * math.pi / N
    hem_ref = float(np.interp(0.0, THS, hem, period=2 * math.pi))
    u = jeans_rows(f, hem_ref)
    legs = []
    for side, mirror in ((0, False), (1, True)):
        order = (list(range(kF, M)) + list(range(0, kB + 1))) if not mirror else list(range(kF, kB - 1, -1))
        p0 = np.concatenate([Pp[-1, order], bridge_P])
        c0 = leg.center(f.z_split)
        x0 = -p0[:, 0] if mirror else p0[:, 0]
        phi0 = np.arctan2(p0[:, 1] - c0[1], x0 - c0[0])
        phi0 = phiu + ((phi0 - phiu + math.pi) % (2 * math.pi) - math.pi)
        z0 = p0[:, 2]
        hj = np.interp(phiu % (2 * math.pi), THS, hem, period=2 * math.pi)
        Z = z0[None] + (hj - z0)[None] * u[:, None]
        PHI = phi0[None] + (phiu - phi0)[None] * smoothstep(0.0, 0.12, u)[:, None]
        P = np.concatenate([p0[None], leg.point(Z[1:], PHI[1:], mirror)])
        sgn = -1.0 if mirror else 1.0
        n = grid_normals(P, lambda z, sgn=sgn: leg.center(z) * [sgn, 1.0])
        d = leg_folds(f, Z, np.broadcast_to(phiu, Z.shape), hj[None], side)
        d[0] = 0.0
        d *= smoothstep(0.0, 0.05, u)[:, None]
        P = P + n * d[..., None]
        legs.append({'P': P, 'n': n, 'order': order, 'Z': Z, 'hem': hj, 'mirror': mirror, 'u': u, 'd': d})
    n = grid_normals(Pp, lambda z: pel.center(z))
    dfly = fly_ridge(f, Pp, PSI, ztop[None], pel)
    Pp = Pp + n * dfly[..., None]
    # Assemble.
    B = Builder()
    Gp = B.verts(Pp, dfly)
    Gb = B.verts(bridge_P)
    nwb = len(WAISTBAND) - 1

    def pel_isl(r, k):
        if r < nwb:
            return J_WAISTBAND_R if 20 <= k < 60 else J_WAISTBAND
        return (J_BACK_L if k < 20 else J_BACK_R if k < 40 else J_FRONT_R if k < 60 else J_FRONT_L)

    B.grid(Gp, wrap=True, isl=pel_isl)
    front_cols = set(range(J_IN, N)) | set(range(0, J_OUT))
    for side, lg in enumerate(legs):
        G = np.concatenate([np.concatenate([Gp[-1, lg['order']], Gb])[None], B.verts(lg['P'][1:], lg['d'][1:])])
        lg['G'] = G
        fr, bk = (J_FRONT_L, J_BACK_L) if side == 0 else (J_FRONT_R, J_BACK_R)
        B.grid(G, wrap=True, isl=lambda r, cc, fr=fr, bk=bk: fr if cc in front_cols else bk)
    # The waistband's top edge rolls over into its facing; each hem rolls up inside.
    # The facings lie parallel to the outside, 3.6 mm in.
    up = np.array([0.0, 0.0, 1.0])
    npl = grid_normals(Pp, lambda z: pel.center(z))
    nt = npl[0]
    top = Pp[0]
    A = B.verts(top + 0.0012 * up - 0.0017 * nt)
    Bv = B.verts(top - 0.0034 * nt + 0.0002 * up)
    facing = B.verts(Pp[1:nwb + 1] - 0.0036 * npl[1:nwb + 1])
    B.grid(np.stack([Gp[0], A, Bv]), wrap=True, isl=lambda r, k: J_WAISTBAND_R if 20 <= k < 60 else J_WAISTBAND)
    B.grid(np.concatenate([Bv[None], facing]), wrap=True, isl=lambda r, k: J_WB_FACING_R if 20 <= k < 60 else J_WB_FACING)
    for side, lg in enumerate(legs):
        Pb, nb = lg['P'][-1], lg['n'][-1]
        D = B.verts(Pb - 0.0012 * up - 0.0017 * nb)
        E = B.verts(Pb - 0.0034 * nb - 0.0002 * up)
        rows_up = lg['P'][-2:-6:-1] - 0.0036 * lg['n'][-2:-6:-1]
        F = B.verts(rows_up)
        fr, bk = (J_FRONT_L, J_BACK_L) if side == 0 else (J_FRONT_R, J_BACK_R)
        B.grid(np.stack([lg['G'][-1], D, E]), wrap=True, isl=lambda r, cc, fr=fr, bk=bk: fr if cc in front_cols else bk)
        hf, hb = (J_HEM_FACING_L, J_HEM_FACING_LB) if side == 0 else (J_HEM_FACING_R, J_HEM_FACING_RB)
        B.grid(np.concatenate([E[None], F]), wrap=True, isl=lambda r, cc, hf=hf, hb=hb: hf if cc in front_cols else hb)
    f.pel, f.leg, f.hem, f.legs, f.Gp, f.Gb, f.psi, f.ztop, f.Pp = pel, leg, hem, legs, Gp, Gb, psi, ztop, Pp
    # What's sewn on: the belt loops, the back pockets, the shank button.
    tree = BVHTree.FromPolygons([tuple(p) for p in B.positions()], B.faces)
    f.r_waist = float(pel.radius(f.z_wb_f - 0.02, THS).mean())
    f.loops = belt_loops(B, f)
    f.pockets = [patch_pocket(B, tree, f, side) for side in (0, 1)]
    f.button = shank_button(B, f)
    obj = B.build(f'SK_Garment_Jeans_{h.key}')
    orient_parts(obj)
    obj.data.shade_smooth()
    return obj, f


def waist_top(f, psi):
    return f.z_wb_f + (f.z_wb_b - f.z_wb_f) * (1.0 + np.sin(psi)) / 2.0


def pel_frame(f, z, psi):
    """A point on the jeans' seat/hips (the pelvis table), its outward normal and the direction round the body."""
    pel = f.pel
    e = 1e-3
    P = pel.point(z, psi)
    dz = pel.point(z + e, psi) - pel.point(z - e, psi)
    dpsi = pel.point(z, psi + e) - pel.point(z, psi - e)
    n = np.cross(dpsi, dz)
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    t = dpsi / np.linalg.norm(dpsi, axis=-1, keepdims=True)
    return P, n, t


def strap(B, path, width_dir, thick, width, isl, mat=0):
    """A flat strap (belt loop) along a path: rounded rectangle section, width along width_dir, capped ends."""
    path = np.asarray(path, float)
    T = np.gradient(path, axis=0)
    T /= np.linalg.norm(T, axis=1, keepdims=True)
    W = np.asarray(width_dir, float) - T * (np.asarray(width_dir, float) * T).sum(1, keepdims=True)
    W /= np.linalg.norm(W, axis=1, keepdims=True)
    Nn = np.cross(W, T)
    rr = thick * 0.45
    prof = []
    for cx, cy, a0 in ((width / 2 - rr, thick / 2 - rr, 0.0), (-width / 2 + rr, thick / 2 - rr, 0.5 * math.pi),
                       (-width / 2 + rr, -thick / 2 + rr, math.pi), (width / 2 - rr, -thick / 2 + rr, 1.5 * math.pi)):
        for k in range(3):
            a = a0 + 0.5 * math.pi * k / 2
            prof.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    rings = np.array([[path[i] + W[i] * x + Nn[i] * y for x, y in prof] for i in range(len(path))])
    G = B.verts(rings)
    B.grid(G, wrap=True, isl=isl, mat=mat)
    for end, rev in ((0, True), (len(path) - 1, False)):
        c = B.verts(rings[end].mean(0)[None])[0]
        m = len(prof)
        for k in range(m):
            a, b = G[end, k], G[end, (k + 1) % m]
            B.face((a, c, b) if rev else (a, b, c), mat, isl)


def belt_loops(B, f):
    """Seven loops: two at the front, one behind each side seam, two at the back, one at center back. Each is sewn under
    the waistband's seam, stands a little proud over the band, and folds over its top edge into the facing."""
    R = f.r_waist
    at = [1.5 * math.pi - 0.095 / R, 1.5 * math.pi + 0.095 / R, 0.14, math.pi - 0.14, 0.5 * math.pi - 0.6, 0.5 * math.pi + 0.6, 0.5 * math.pi]
    out = []
    for psi in at:
        zt = float(waist_top(f, psi))
        zs = np.linspace(zt - WAISTBAND[-1] - 0.009, zt - 0.0005, 12)
        P, n, t = pel_frame(f, zs, np.full_like(zs, psi))
        s = np.linspace(0.0, 1.0, len(zs))
        off = 0.0003 + 0.0026 * np.sin(math.pi * np.clip(s * 1.15, 0, 1)) ** 0.6 + 0.0011
        path = list(P + n * off[:, None])
        up = np.array([0.0, 0.0, 1.0])
        top, nt = P[-1], n[-1]
        path += [top + up * 0.0026 + nt * 0.0012, top + up * 0.0029 - nt * 0.0012, top + up * 0.0018 - nt * 0.0042,
                 top - up * 0.004 - nt * 0.0052]
        wd = np.vstack([t, np.repeat(t[-1:], 4, 0)])
        strap(B, path, wd, 0.0022, 0.0105, J_LOOP)
        out.append(psi)
    return out


def yoke_depth(psi):
    """How far the back yoke's seam is below the waistband's seam: 7.5 cm at center back, 3 cm at the side seams."""
    return 0.075 - 0.045 * np.clip(np.abs(((np.asarray(psi) - 0.5 * math.pi + math.pi) % (2 * math.pi)) - math.pi) / (0.5 * math.pi), 0.0, 1.0)


def pocket_outline(f):
    """The back pocket's size: width, side height, height at its point (scaled with the seat)."""
    seat = 2 * math.pi * float(f.pel.radius(f.z_split + 0.05, THS).mean())
    w = min(max(0.135 * seat / 1.05, 0.13), 0.155)
    return w, 0.135 * w / 0.14, 0.155 * w / 0.14


def patch_pocket(B, tree, f, side):
    """A five-sided patch pocket on the seat, sewn 1.6 mm proud of the back panel, with its edge turned under."""
    sgn = 1.0 if side == 0 else -1.0
    psi_c = 0.5 * math.pi - sgn * 0.66
    w, hs, hp = pocket_outline(f)
    z_top = float(waist_top(f, psi_c)) - WAISTBAND[-1] - float(yoke_depth(psi_c)) - 0.014
    R = float(f.pel.radius(z_top - 0.07, psi_c))
    ns, nt = 19, 17
    S, Tt = np.meshgrid(np.linspace(-1.0, 1.0, ns), np.linspace(0.0, 1.0, nt))
    H = hs + (hp - hs) * (1.0 - np.abs(S))
    a = S * w / 2 * (1.0 - 0.05 * Tt) - 0.009 * Tt * Tt  # the point leans toward center back
    b = Tt * H
    psi = psi_c - sgn * a / R          # +a toward the side seam
    z = z_top - b
    rough = f.pel.point(z, psi)
    P = np.zeros_like(rough)
    n = np.zeros_like(rough)
    for idx in np.ndindex(S.shape):
        loc, nor, _, _ = tree.find_nearest(Vector(rough[idx]))
        nor = np.array(nor)
        radial_d = np.array([loc.x, loc.y - f.pel.center(loc.z)[1], 0.0])
        if nor @ radial_d < 0:
            nor = -nor
        P[idx], n[idx] = np.array(loc), nor
    n = smooth_grid(n)
    G = B.verts(P + n * 0.0017)
    isl = J_POCKET_L if side == 0 else J_POCKET_R
    B.grid(G, isl=isl)
    loop = list(G[0, :]) + list(G[1:, -1]) + list(G[-1, -2::-1]) + list(G[-2:0:-1, 0])
    loopP = np.array([B.positions()[i] for i in loop])
    loopN = np.concatenate([n[0, :], n[1:, -1], n[-1, -2::-1], n[-2:0:-1, 0]])
    inner = B.verts(loopP - loopN * 0.0024)
    for k in range(len(loop)):
        k1 = (k + 1) % len(loop)
        B.face((loop[k1], loop[k], inner[k], inner[k1]), 0, isl)
    return {'psi': psi_c, 'z_top': z_top, 'w': w, 'hs': hs, 'hp': hp, 'R': R, 'side': side}


def smooth_grid(n, it=2):
    for _ in range(it):
        m = n.copy()
        m[1:-1, 1:-1] = (n[1:-1, 1:-1] * 4 + n[:-2, 1:-1] + n[2:, 1:-1] + n[1:-1, :-2] + n[1:-1, 2:]) / 8
        n = m / np.linalg.norm(m, axis=-1, keepdims=True)
    return n


def shank_button(B, f):
    """The waistband's shank button, through the buttonhole at the overlap's end (the wearer's left of center)."""
    psi = 1.5 * math.pi + 0.013 / f.r_waist
    z = float(waist_top(f, psi)) - WAISTBAND[-1] / 2
    P, n, t = pel_frame(f, np.array([z]), np.array([psi]))
    P, n, t = P[0], n[0], t[0]
    up = np.cross(n, t)
    prof = [(0.0, 0.0002), (0.0074, 0.0002), (0.0085, 0.0011), (0.0086, 0.0027), (0.0079, 0.0036), (0.0064, 0.0041),
            (0.0045, 0.0039), (0.0025, 0.0043), (0.0, 0.0045)]
    seg = 28
    ang = np.linspace(0.0, 2 * math.pi, seg, endpoint=False)
    rings = []
    for r, hgt in prof[1:-1]:
        rings.append([P + n * (hgt + 0.0018) + (t * math.cos(a) + up * math.sin(a)) * r for a in ang])
    G = B.verts(np.array(rings))
    B.grid(G, wrap=True, mat=1, isl=J_BUTTON)
    bottom = B.verts((P + n * (prof[0][1] + 0.0018))[None])[0]
    top = B.verts((P + n * (prof[-1][1] + 0.0018))[None])[0]
    for k in range(seg):
        k1 = (k + 1) % seg
        B.face((bottom, G[0, k1], G[0, k]), 1, J_BUTTON)
        B.face((top, G[-1, k], G[-1, k1]), 1, J_BUTTON)
    return {'P': P, 'n': n}


def orient_parts(obj):
    """Consistent normals per connected part: closed parts (loops, button) outward by their own shape, open ones
    (the jeans, the pockets) away from the body's vertical axis."""
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.faces.ensure_lookup_table()
    seen = set()
    for f0 in bm.faces:
        if f0.index in seen:
            continue
        comp, stack = [], [f0]
        seen.add(f0.index)
        while stack:
            fa = stack.pop()
            comp.append(fa)
            for e in fa.edges:
                for fb in e.link_faces:
                    if fb.index not in seen:
                        seen.add(fb.index)
                        stack.append(fb)
        bmesh.ops.recalc_face_normals(bm, faces=comp)
        closed = all(not e.is_boundary for fa in comp for e in fa.edges)
        if closed:
            continue
        s = 0.0
        for fa in comp:
            c = fa.calc_center_median()
            s += (fa.normal.x * c.x + fa.normal.y * c.y) * fa.calc_area()
        if s < 0:
            bmesh.ops.reverse_faces(bm, faces=comp)
    bm.to_mesh(me)
    bm.free()
    me.update()


def orient_outward(obj):
    """Consistent face normals, pointing away from the body's vertical axis on the whole (flipped if most don't)."""
    outward_normals(obj)
    me = obj.data
    me.update()
    n = np.empty(len(me.polygons) * 3)
    me.polygons.foreach_get('normal', n)
    c = np.empty(len(me.polygons) * 3)
    me.polygons.foreach_get('center', c)
    n, c = n.reshape(-1, 3), c.reshape(-1, 3)
    c[:, 2] = 0.0
    if (n * c).sum() < 0:
        bm = bmesh.new()
        bm.from_mesh(me)
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        bm.to_mesh(me)
        bm.free()
        me.update()


# ------------------------------------------------------------------ previews

def preview(objs, path, views=(('front', 0.0), ('side', 90.0), ('back', 180.0), ('three', -35.0)), size=700, center=None, span=None):
    """Quick solid renders (Workbench) side by side, for iterating on shape."""
    sc = bpy.context.scene
    old_engine = sc.render.engine
    sc.render.engine = 'BLENDER_WORKBENCH'
    sc.display.shading.light = 'STUDIO'
    sc.display.shading.color_type = 'OBJECT'
    sc.display.shading.show_cavity = True
    sc.display.shading.cavity_type = 'WORLD'
    hidden = []
    for o in sc.objects:
        if o.type == 'MESH' and o not in objs and not o.hide_render:
            o.hide_render = True
            hidden.append(o)
    lo, hi = core.bounds(objs)
    ctr = Vector(center) if center else (lo + hi) / 2
    ext = span or max(hi.z - lo.z, hi.x - lo.x) * 1.05
    cam = core.link(bpy.data.objects.new('prev_cam', bpy.data.cameras.new('prev_cam')))
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = ext
    sc.camera = cam
    sc.render.resolution_x = size
    sc.render.resolution_y = size
    tiles = []
    for view in views:
        label, az = view[0], view[1]
        el = math.radians(view[2]) if len(view) > 2 else 0.0
        a = math.radians(az)
        d = Vector((math.sin(a) * math.cos(el), -math.cos(a) * math.cos(el), math.sin(el)))
        cam.location = ctr + d * 5.0
        cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
        p = os.path.join(core.BUILD_DIR, 'review', 'garments', f'_prev_{label}.png')
        os.makedirs(os.path.dirname(p), exist_ok=True)
        sc.render.filepath = p
        bpy.ops.render.render(write_still=True)
        tiles.append(p)
    bpy.data.objects.remove(cam)
    for o in hidden:
        o.hide_render = False
    sc.render.engine = old_engine
    out = np.concatenate([review_pixels(p) for p in tiles], axis=1)
    save_rgb(path, out)
    return path


def review_pixels(path):
    img = bpy.data.images.load(path, check_existing=False)
    w, hh = img.size
    a = np.empty(w * hh * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return a.reshape(hh, w, 4)


def save_rgb(path, px, quality=90):
    hh, w = px.shape[:2]
    img = bpy.data.images.new('_save', w, hh, alpha=False)
    full = np.ones((hh, w, 4), dtype=np.float32)
    full[:, :, :3] = px[:, :, :3]
    img.pixels.foreach_set(full.ravel())
    img.filepath_raw = path
    if path.lower().endswith('.jpg'):
        img.file_format = 'JPEG'
        img.save(quality=quality)
    else:
        img.file_format = 'PNG'
        img.save()
    bpy.data.images.remove(img)


def hide_toes(h):
    """As the game does in the shoes (HideBone on ball_l/_r): the toes shrink to the ball bone's parent."""
    for s in ('l', 'r'):
        pb = h.arm.pose.bones.get(f'ball_{s}')
        if pb:
            pb.scale = (0.001, 0.001, 0.001)
    bpy.context.view_layer.update()


# ------------------------------------------------------------------ main

def args():
    a = sys.argv
    return a[a.index('--') + 1:] if '--' in a else []


def main():
    a = args()
    wardrobe = a[a.index('--wardrobe') + 1] if '--wardrobe' in a else WARDROBE_DEFAULT
    keys = [k for k in a if k in HEROES] or list(HEROES)
    for key in keys:
        t0 = time.time()
        core.reset()
        h = Hero(key, wardrobe)
        shoes = put_on_shoes(h)
        jeans, f = build_jeans(h, shoes)
        print(f'[garments] {key}: jeans {len(jeans.data.vertices)} verts, {len(jeans.data.polygons)} faces, crotch {f.z_crotch:.3f} '
              f'split {f.z_split:.3f} waist {f.z_wb_f:.3f}/{f.z_wb_b:.3f} knee L {f.L_knee:.3f} hem L {f.L_hem:.3f} ({time.time() - t0:.0f}s)')
        h.body.color = (0.75, 0.6, 0.5, 1.0)
        jeans.color = (0.25, 0.35, 0.6, 1.0)
        if '--jacket-preview' in a:
            hide_toes(h)
            jacket, k = build_jacket(h, f, jeans)
            jacket.color = (0.55, 0.42, 0.3, 1.0)
            print(f'[garments] {key}: jacket {len(jacket.data.vertices)} verts, {len(jacket.data.polygons)} faces; hem {k.z_hem:.3f} '
                  f'armpit {k.z_a:.3f} shoulder {k.z_S:.3f} neck {k.z_neck_min:.3f} armholes {k.holes}')
            d = os.path.join(core.BUILD_DIR, 'review', 'garments')
            preview([h.body, jeans, jacket] + shoes, os.path.join(d, f'jacket_{key}.png'))
            preview([h.body, jeans, jacket] + shoes, os.path.join(d, f'jacket_{key}_top.png'), center=(0.0, 0.0, 1.2), span=0.8,
                    views=(('front', 0.0), ('side', 90.0), ('back', 180.0), ('three', -35.0, 25.0)))
            continue
        if '--preview' in a:
            hide_toes(h)
            preview([h.body, jeans] + shoes, os.path.join(core.BUILD_DIR, 'review', 'garments', f'jeans_{key}.png'))
            preview([h.body, jeans] + shoes, os.path.join(core.BUILD_DIR, 'review', 'garments', f'jeans_{key}_low.png'),
                    center=(0.0, 0.0, 0.25), span=0.6)
            preview([h.body, jeans] + shoes, os.path.join(core.BUILD_DIR, 'review', 'garments', f'jeans_{key}_waist.png'),
                    center=(0.0, 0.0, f.z_split + 0.09), span=0.5)
            continue
        finish_jeans(h, jeans, f, bake='--no-bake' not in a)
        hide_toes(h)
        if '--look' in a:
            look(h, [h.body, jeans] + shoes, os.path.join(core.BUILD_DIR, 'review', 'garments', f'look_jeans_{key}.jpg'),
                 [('front', 0, 8, 2.7, (0, 0, 0.62)), ('back', 180, 8, 2.7, (0, 0, 0.62)), ('seat', 160, 10, 1.1, (0, 0.05, 0.86)),
                  ('fly', -20, 12, 1.0, (0, -0.1, 0.88)), ('hem', 30, 18, 1.0, (0.12, -0.05, 0.12)), ('knee', -60, 5, 1.2, (0.1, 0, 0.45))])
        # The jacket is fitted over the jeans as they stand, before they're bound to the armature.
        garments = [jeans]
        k = None
        if '--jeans-only' not in a:
            jacket, k = build_jacket(h, f, jeans)
            print(f'[garments] {key}: jacket {len(jacket.data.vertices)} verts, {len(jacket.data.polygons)} faces ({time.time() - t0:.0f}s)')
            finish_jacket(h, jacket, k, bake='--no-bake' not in a)
            garments.append(jacket)
        skin_jeans(h, jeans, f)
        if k is not None:
            skin_jacket(h, jacket, k, jeans)
        for g in garments:
            strip_bake_attrs(g)
            print(f'[garments] {g.name}: {len(g.data.polygons)} faces, {count_tris(g)} triangles, skinned to {len(g.vertex_groups)} bones, '
                  f'{len(g.material_slots)} material slots ({time.time() - t0:.0f}s)')
            if '--no-export' not in a:
                path = export_fbx(h, g, os.path.join(GARMENT_DIR, f'{g.name}.fbx'))
                print(f'[garments] exported {os.path.relpath(path, core.ROOT)}, textures {copy_textures(g.name, len(g.material_slots))}')
                write_notes(g.name, f.uv_scale if g is jeans else k.uv_scale)
        if '--no-export' not in a:
            print(f'[garments] weave details {write_details()}')
        report = validate(h, garments, f, k)
        if k is not None:
            review_tint(jacket, OUTFIT_TONES['olive' if key == 'A' else 'burgundy'])
        if '--no-review' not in a:
            review_sheet(h, [h.body] + garments + shoes, os.path.join(core.REVIEW_DIR, f'garments_{key}.jpg'))
            detail_sheet(h, [h.body] + garments + shoes, os.path.join(core.REVIEW_DIR, f'garments_{key}_detail.jpg'), f, k)
        if '--no-review' not in a:
            compose_overview()
        print(f'[garments] {key}: done ({time.time() - t0:.0f}s) {report}')


def count_tris(obj):
    obj.data.calc_loop_triangles()
    return len(obj.data.loop_triangles)


def validate(h, garments, f, k=None):
    """How much comes through each garment in each review pose: the legs through the jeans, the arms through the
    sleeves, the jeans through the jacket (vertices, worst depth in mm)."""
    jeans = garments[0]
    jacket = garments[1] if len(garments) > 1 else None
    ankle = float(h.bone('foot_l')[2]) + 0.03

    def legs(R):
        # Above the hem at each vertex's own angle round its leg (the hem's front rides up over the shoe).
        sx = np.where(R[:, 0] >= 0, 1.0, -1.0)
        c = f.leg.center(R[:, 2])
        ph = np.mod(np.arctan2(R[:, 1] - c[:, 1], sx * R[:, 0] - c[:, 0]), 2 * math.pi)
        hz = np.interp(ph, THS, f.hem, period=2 * math.pi)
        return (R[:, 2] < f.z_split) & (R[:, 2] > np.maximum(hz + 0.03, ankle)) & (np.abs(R[:, 0]) < 0.3)

    def arms(R):
        # The bare arm up to two fingers short of the wrist (the cuff's end).
        ok = np.zeros(len(R), bool)
        for s, sg in (('l', 1.0), ('r', -1.0)):
            wrist, elbow = h.bone(f'hand_{s}'), h.bone(f'lowerarm_{s}')
            fd = (wrist - elbow) / np.linalg.norm(wrist - elbow)
            ok |= (sg * R[:, 0] > 0.12) & (R[:, 2] > 0.85) & ((R - wrist) @ fd < -0.03)
        return ok

    def under_jacket(R):
        return (R[:, 2] > k.z_hem + 0.012) & (R[:, 2] < k.z_a)

    out = {}
    for kind in ('stand', 'walk', 'sit', 'reach'):
        pose(h, kind)
        n, worst, total = pokes(h, jeans, legs, skip=set(J_INSIDE))
        line = f'legs/jeans {n}/{total} ({worst * 1000:.1f} mm)'
        if jacket is not None:
            sleeves = {K_SLEEVE_LF, K_SLEEVE_LB, K_SLEEVE_RF, K_SLEEVE_RB, K_CUFF_LF, K_CUFF_LB, K_CUFF_RF, K_CUFF_RB}
            body_parts = {K_FRONT_L, K_BACK, K_FRONT_R, K_BAND_L, K_BAND_B, K_BAND_R, K_WELT_L, K_WELT_R}
            n2, w2, t2 = pokes(h, jacket, arms, skip=set(range(32)) - sleeves)
            n3, w3, t3 = pokes(h, jacket, under_jacket, skip=set(range(32)) - body_parts, source=jeans)
            line += f', arms/sleeves {n2}/{t2} ({w2 * 1000:.1f} mm), jeans/jacket {n3}/{t3} ({w3 * 1000:.1f} mm)'
        out[kind] = line
        print(f'[garments] {h.key} {kind}: {line}')
    pose_reset(h)
    return out


def review_sheet(h, objs, path, poses=('stand', 'walk', 'sit', 'reach'), tile=560):
    """Rows of poses, each from the front, side and back, rendered as the review rig renders (Cycles)."""
    tiles = []
    for kind in poses:
        pose(h, kind)
        sit = kind == 'sit'
        tz = 0.62 if sit else 0.82
        if sit:
            shift(h, 'pelvis', (0.0, 0.0, -0.4))
        views = [('front', -12, 6, 2.9, (0, -0.05 if not sit else -0.2, tz)), ('side', 78, 6, 2.9, (0, 0, tz)), ('back', 168, 6, 2.9, (0, 0.02, tz))]
        p = look(h, objs, os.path.join(core.BUILD_DIR, 'review', 'garments', f'_sheet_{kind}.png'), views, tile=tile, samples=48)
        tiles.append(review_pixels(p))
    pose_reset(h)
    out = np.concatenate(tiles[::-1], axis=0)
    save_rgb(path, out)
    print(f'[garments] review sheet {os.path.relpath(path, core.ROOT)}')
    return path


def shift(h, name, offset):
    pb = h.arm.pose.bones[name]
    bpy.context.view_layer.update()
    k = 1.0 / h.arm.matrix_world.to_scale()[0]
    pb.matrix = Matrix.Translation(Vector(offset) * k) @ pb.matrix
    bpy.context.view_layer.update()


def finish_jeans(h, jeans, f, bake=True):
    """UVs, the painted denim and its bake (slot 0), the button's metal (slot 1)."""
    name = jeans.name
    for m in (core.Mat(f'{name}_denim_placeholder').m, metal_material(f'M_{name}_button', 0x7a6a58, 0.38)):
        jeans.data.materials.append(m)
    unwrap(jeans)
    f.uv_scale = layout_uvs(jeans, {0: TEXTURE_SIZE, 1: 512})
    paint = jeans_textures(jeans, f, name)
    jeans.material_slots[0].material = texture_material(f'M_{name}_denim', paint, TEXTURE_SIZE, specular=0.28, sheen=0.0)
    if bake:
        bake_garment(jeans, name, {jeans.material_slots[1].material.name: 512}, ao_distance=0.025)


def bake_garment(obj, name, sizes, ao_distance=0.02):
    """The pipeline's bake (core.bake: BaseColor, ORM, Normal per slot) with only the garment in the scene's render."""
    hidden = [o for o in bpy.context.scene.objects if o is not obj and not o.hide_render]
    for o in hidden:
        o.hide_render = True
    mats = core.bake(obj, name, size=TEXTURE_SIZE, ao_distance=ao_distance, sizes=sizes)
    for o in hidden:
        o.hide_render = False
    for m in mats:
        m.use_backface_culling = False
    return mats


# ------------------------------------------------------------------ look renders (Cycles)

def look(h, objs, path, views, tile=640, samples=64):
    """Renders the garments on the body under a soft studio rig: each view (label, azimuth, elevation, distance, target)."""
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = core.cycles_device()
    sc.world.color = (0.012, 0.013, 0.016)
    hidden = [o for o in sc.objects if o.type == 'MESH' and o not in objs and not o.hide_render]
    for o in hidden:
        o.hide_render = True
    lights = []
    for nm, loc, power, size, col in (('key', (-1.6, -2.4, 2.6), 380.0, 1.6, (1.0, 0.95, 0.88)),
                                      ('fill', (2.4, -1.6, 1.2), 110.0, 2.4, (0.82, 0.88, 1.0)),
                                      ('rim', (0.6, 2.6, 2.4), 420.0, 1.2, (1.0, 1.0, 1.0)),
                                      ('low', (0.0, -2.0, 0.15), 45.0, 1.5, (1.0, 0.97, 0.92))):
        d = bpy.data.lights.new(nm, 'AREA')
        d.shape = 'DISK'
        d.size = size
        d.energy = power
        d.color = col
        o = core.link(bpy.data.objects.new(nm, d))
        o.location = loc
        o.rotation_euler = (Vector((0.0, 0.0, 0.9)) - Vector(loc)).to_track_quat('-Z', 'Y').to_euler()
        o.visible_camera = False
        lights.append(o)
    bpy.ops.mesh.primitive_plane_add(size=12.0, location=(0.0, 0.0, h.floor))
    floor = bpy.context.view_layer.objects.active
    fm = core.Mat('look_floor')
    fm.set('Base Color', core.hex_linear(0x24262b))
    fm.set('Roughness', 0.55)
    core.assign(floor, fm)
    cam = core.link(bpy.data.objects.new('look_cam', bpy.data.cameras.new('look_cam')))
    cam.data.lens = 50
    sc.camera = cam
    tiles = []
    for v in views:
        label, az, el, dist, tgt = v
        a, e = math.radians(az), math.radians(el)
        t = Vector(tgt)
        cam.location = t + Vector((math.sin(a) * math.cos(e), -math.cos(a) * math.cos(e), math.sin(e))) * dist
        cam.rotation_euler = (t - cam.location).to_track_quat('-Z', 'Y').to_euler()
        p = os.path.join(core.BUILD_DIR, 'review', 'garments', f'_look_{label}.png')
        sc.render.resolution_x = tile
        sc.render.resolution_y = tile
        sc.cycles.samples = samples
        sc.cycles.use_denoising = True
        sc.render.filepath = p
        sc.render.image_settings.file_format = 'PNG'
        bpy.ops.render.render(write_still=True)
        tiles.append(review_pixels(p))
    cols = 3
    rows = (len(tiles) + cols - 1) // cols
    out = np.full((rows * tile, cols * tile, 4), 0.05, dtype=np.float32)
    for k, im in enumerate(tiles):
        r, c = k // cols, k % cols
        out[(rows - 1 - r) * tile:(rows - r) * tile, c * tile:(c + 1) * tile] = im
    save_rgb(path, out)
    for o in lights + [cam, floor]:
        bpy.data.objects.remove(o)
    for o in hidden:
        o.hide_render = False
    return path



# ------------------------------------------------------------------ UVs: pattern pieces at true scale

def mark_seams(obj):
    """UV seams wherever two pattern pieces (islands) or two material slots meet."""
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    lay = bm.faces.layers.int.get('island')
    for e in bm.edges:
        fs = e.link_faces
        e.seam = len(fs) != 2 or fs[0][lay] != fs[1][lay] or fs[0].material_index != fs[1].material_index
    bm.to_mesh(me)
    bm.free()


def unwrap(obj):
    mark_seams(obj)
    if not obj.data.uv_layers:
        obj.data.uv_layers.new(name='UVMap')
    core.select_only([obj])
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    try:
        bpy.ops.uv.unwrap(method='MINIMUM_STRETCH', margin=0.0)
    except (TypeError, RuntimeError):
        bpy.ops.uv.unwrap(method='CONFORMAL', margin=0.0)
    bpy.ops.object.mode_set(mode='OBJECT')


def face_arrays(obj):
    me = obj.data
    nf = len(me.polygons)
    ls = np.empty(nf, dtype=np.int64)
    me.polygons.foreach_get('loop_start', ls)
    lt = np.empty(nf, dtype=np.int64)
    me.polygons.foreach_get('loop_total', lt)
    mi = np.empty(nf, dtype=np.int64)
    me.polygons.foreach_get('material_index', mi)
    isl = np.empty(nf, dtype=np.int64)
    me.attributes['island'].data.foreach_get('value', isl)
    area = np.empty(nf)
    me.polygons.foreach_get('area', area)
    lv = np.empty(len(me.loops), dtype=np.int64)
    me.loops.foreach_get('vertex_index', lv)
    order = np.argsort(ls)
    fl = np.empty(len(me.loops), dtype=np.int64)
    for f in order:
        fl[ls[f]:ls[f] + lt[f]] = f
    return ls, lt, mi, isl, area, lv, fl


def layout_uvs(obj, sizes, margin_px=12, upright=()):
    """Pattern pieces flattened at true scale (meters), each turned so the garment's up is +V (the grain runs along V,
    print reads upright), then shelf-packed per material slot into its own 0..1 square. Returns {slot: UV units per meter}."""
    me = obj.data
    uvl = me.uv_layers.active.data
    uv = np.empty(len(me.loops) * 2)
    uvl.foreach_get('uv', uv)
    uv = uv.reshape(-1, 2)
    ls, lt, mi, isl, area, lv, fl = face_arrays(obj)
    co = get_co(obj)
    lmat, lisl = mi[fl], isl[fl]
    pieces = []
    for key in sorted(set(zip(mi.tolist(), isl.tolist()))):
        m, i = key
        loops = np.nonzero((lmat == m) & (lisl == i))[0]
        faces = np.nonzero((mi == m) & (isl == i))[0]
        # Separate connected pieces sharing an island id (the belt loops) are packed one by one.
        for comp in uv_components(faces, ls, lt, lv):
            cl = np.concatenate([np.arange(ls[f], ls[f] + lt[f]) for f in comp])
            u = uv[cl].copy()
            a_uv = 0.0
            for f in comp:
                p = uv[ls[f]:ls[f] + lt[f]]
                a_uv += 0.5 * np.sum(p[:, 0] * np.roll(p[:, 1], -1) - np.roll(p[:, 0], -1) * p[:, 1])
            if a_uv < 0:
                u[:, 0] = -u[:, 0]
                a_uv = -a_uv
            k = math.sqrt(area[comp].sum() / max(a_uv, 1e-12))
            u = (u - u.mean(0)) * k
            z = co[lv[cl], 2]
            A = np.stack([u[:, 0], u[:, 1], np.ones(len(u))], 1)
            (ga, gb, _), *_ = np.linalg.lstsq(A, z, rcond=None)
            g = math.hypot(ga, gb)
            if g > 0.2:
                al = math.atan2(ga, gb)
                ca, sa = math.cos(al), math.sin(al)
                u = np.stack([u[:, 0] * ca - u[:, 1] * sa, u[:, 0] * sa + u[:, 1] * ca], 1)
            u -= u.min(0)
            pieces.append({'mat': m, 'isl': i, 'loops': cl, 'uv': u, 'size': u.max(0), 'faces': comp})
    scale = {}
    for m in sorted(set(p['mat'] for p in pieces)):
        ps = [p for p in pieces if p['mat'] == m]
        size = sizes.get(m, TEXTURE_SIZE)
        gap = margin_px / size
        lo, hi = 0.01, 50.0
        for _ in range(50):
            mid = (lo + hi) / 2
            if shelf_pack([p['size'] * mid for p in ps], gap) is not None:
                lo = mid
            else:
                hi = mid
        pos = shelf_pack([p['size'] * lo for p in ps], gap)
        for p, (x, y) in zip(ps, pos):
            uv[p['loops']] = p['uv'] * lo + [x, y]
            p['offset'] = (x, y)
        scale[m] = lo
    uvl.foreach_set("uv", uv.ravel())
    piece = np.zeros(len(mi), dtype=np.int32)
    for n_p, p in enumerate(pieces):
        piece[p["faces"]] = n_p
    if "piece" in me.attributes:
        me.attributes.remove(me.attributes["piece"])
    me.attributes.new("piece", "INT", "FACE").data.foreach_set("value", piece)
    obj["uv_pieces"] = [[int(p["mat"]), int(p["isl"]), float(p["offset"][0]), float(p["offset"][1])] for p in pieces]
    me.update()
    return scale


def uv_components(faces, ls, lt, lv):
    """Faces split into groups connected through shared vertices."""
    fset = set(faces.tolist())
    by_v = {}
    for f in faces:
        for v in lv[ls[f]:ls[f] + lt[f]]:
            by_v.setdefault(int(v), []).append(int(f))
    seen = set()
    out = []
    for f0 in faces:
        f0 = int(f0)
        if f0 in seen:
            continue
        comp, stack = [], [f0]
        seen.add(f0)
        while stack:
            f = stack.pop()
            comp.append(f)
            for v in lv[ls[f]:ls[f] + lt[f]]:
                for g in by_v[int(v)]:
                    if g not in seen and g in fset:
                        seen.add(g)
                        stack.append(g)
        out.append(np.array(comp))
    return out


def shelf_pack(sizes, gap):
    order = sorted(range(len(sizes)), key=lambda i: -sizes[i][1])
    x = y = shelf = 0.0
    pos = [None] * len(sizes)
    for i in order:
        w, h = sizes[i]
        if w + 2 * gap > 1.0:
            return None
        if x + w + 2 * gap > 1.0:
            y += shelf + gap
            x = shelf = 0.0
        if y + h + 2 * gap > 1.0:
            return None
        pos[i] = (x + gap, y + gap)
        x += w + gap
        shelf = max(shelf, h)
    return pos


# ------------------------------------------------------------------ per-texel attributes

def set_attr(obj, name, values, domain='POINT'):
    me = obj.data
    if name in me.attributes:
        me.attributes.remove(me.attributes[name])
    a = me.attributes.new(name, 'FLOAT_COLOR', domain)
    v = np.ones((len(values), 4), dtype=np.float32)
    v[:, :3] = values
    a.data.foreach_set('color', v.ravel())


def bake_attrs(obj, size, names, slot=0):
    """Bakes color attributes (as stored, linear) to size x size float maps, rows bottom-up, for the faces of one
    material slot; plus 'valid', the texels its faces cover."""
    sc = bpy.context.scene
    saved = [s.material for s in obj.material_slots]
    scratch = bpy.data.images.new('_bake_scratch', size, size, alpha=False, float_buffer=True)
    out = {}
    sc.render.bake.margin = 0
    sc.render.bake.use_clear = True
    sc.cycles.samples = 1
    for name in list(names) + ['valid']:
        img = bpy.data.images.new(f'_bake_{name}', size, size, alpha=False, float_buffer=True)
        img.colorspace_settings.name = 'Non-Color'
        for i, s in enumerate(obj.material_slots):
            m = core.Mat(f'_bake_{name}_{i}')
            em = m.node('ShaderNodeEmission')
            if name == 'valid':
                em.inputs['Color'].default_value = (1.0, 1.0, 1.0, 1.0)
            else:
                at = m.node('ShaderNodeAttribute')
                at.attribute_name = name
                m.link(at.outputs['Color'], em.inputs['Color'])
            m.link(em.outputs['Emission'], m.out.inputs['Surface'])
            tex = m.nt.nodes.new('ShaderNodeTexImage')
            tex.image = img if i == slot else scratch
            m.nt.nodes.active = tex
            s.material = m.m
        core.select_only([obj])
        bpy.ops.object.bake(type='EMIT')
        a = np.empty(size * size * 4, dtype=np.float32)
        img.pixels.foreach_get(a)
        out[name] = a.reshape(size, size, 4)[:, :, :3].copy()
        bpy.data.images.remove(img)
    bpy.data.images.remove(scratch)
    for s, m in zip(obj.material_slots, saved):
        s.material = m
    out['valid'] = out['valid'][:, :, 0] > 0.5
    return out


def corner_values(obj, per_face):
    """Per-face values (F x 3) spread to every corner of the face."""
    _, _, _, _, _, _, fl = face_arrays(obj)
    return np.asarray(per_face, float)[fl]


# ------------------------------------------------------------------ texture painting helpers

def dash(s, period, duty, soft):
    """1 on a stitch, 0 between: along-line coordinate s (m)."""
    t = np.mod(s, period) / period
    e = soft / period
    return smoothstep(0.0, e, t) * (1.0 - smoothstep(duty - e, duty, t))


def stitch_row(d, s, width=0.0009, period=0.0034, duty=0.72):
    """A row of lockstitches along a line: d the signed distance from the line, s the distance along it (m).
    Returns (thread coverage 0..1, height in m)."""
    across = 1.0 - smoothstep(width * 0.35, width * 0.65, np.abs(d))
    along = dash(s, period, duty, 0.0004)
    cov = across * along
    # Each stitch rounds up over the cloth and pulls it into a dimple where the needle went through.
    prof = np.sqrt(np.clip(1.0 - (d / (width * 0.6)) ** 2, 0.0, 1.0))
    hole = np.exp(-((np.mod(s, period) - period * (duty + 1.0) / 2.0) / (period * 0.12)) ** 2) * np.exp(-(d / width) ** 2)
    return cov, 0.00028 * prof * along * across - 0.00012 * hole


def soft_line(d, w):
    return 1.0 - smoothstep(w * 0.5, w, np.abs(d))


def wrap_pi(a):
    return (a + math.pi) % (2 * math.pi) - math.pi


def mix3(a, b, t):
    t = np.asarray(t, float)[..., None]
    return a * (1.0 - t) + b * t


def lin(h):
    return np.array(core.hex_linear(h)[:3])



# ------------------------------------------------------------------ the jeans' denim

INDIGO = 0x1f2c47       # rinse-washed indigo, the darkest the cloth gets
INDIGO_WORN = 0x6e86aa  # where it's faded through to the white weft
DENIM_BACK = 0x8794ad   # the cloth's inside (weft side)
THREAD = 0xc98f3e       # tobacco topstitching
LEATHER = 0x8d5c33
COPPER = 0xa4673f


def patch_artwork():
    """The leather patch's debossed lettering as a white-on-black mask (65 x 31 mm), rows bottom-up."""
    from artkit.sheet import Sheet
    s = Sheet(65.0, 31.0)
    s.rect(0, 0, 65, 31, 0x000000)
    s.text('FIFTH STREET', 32.5, 17.5, 7.4, 0xffffff, face='Black', align='CENTER', tracking=1.05)
    s.text('DENIM CO.', 32.5, 9.6, 4.6, 0xffffff, face='Bold', align='CENTER', tracking=1.2)
    s.text('NORTHSIDE  ·  EST. 1979', 32.5, 4.0, 2.6, 0xffffff, face='Medium', align='CENTER', tracking=1.1)
    for x0, y0, x1, y1 in ((2.2, 2.2, 62.8, 2.9), (2.2, 28.1, 62.8, 28.8), (2.2, 2.2, 2.9, 28.8), (62.1, 2.2, 62.8, 28.8),
                           (8.0, 14.6, 57.0, 15.1)):
        s.rect(x0, y0, x1 - x0, y1 - y0, 0xffffff)
    color, _ = s.render('garment_patch', 520, samples=16)
    img = bpy.data.images.load(color, check_existing=False)
    w, hh = img.size
    a = np.empty(w * hh * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    for o in [o for o in bpy.data.objects if o.name.startswith('sheet_cam')]:
        bpy.data.objects.remove(o)
    return a.reshape(hh, w, 4)[:, :, 0]


def sample_img(img, u, v):
    """Bilinear lookup in a (rows bottom-up) image at u, v in 0..1 (0 outside)."""
    hh, w = img.shape
    x = np.clip(u * (w - 1), 0, w - 1.001)
    y = np.clip(v * (hh - 1), 0, hh - 1.001)
    x0, y0 = np.floor(x).astype(int), np.floor(y).astype(int)
    fx, fy = x - x0, y - y0
    out = (img[y0, x0] * (1 - fx) * (1 - fy) + img[y0, x0 + 1] * fx * (1 - fy) + img[y0 + 1, x0] * (1 - fx) * fy
           + img[y0 + 1, x0 + 1] * fx * fy)
    inside = (u >= 0) & (u <= 1) & (v >= 0) & (v <= 1)
    return np.where(inside, out, 0.0)


def jeans_textures(obj, f, name, size=TEXTURE_SIZE):
    """Paints the denim (slot 0) per texel from where each texel is on the garment: returns linear base color, roughness,
    metallic and height (m) images, the texel positions and coverage."""
    t0 = time.time()
    co = get_co(obj)
    nrm = vertex_normals(obj)
    fold = np.empty(len(obj.data.vertices), dtype=np.float32)
    obj.data.attributes['fold'].data.foreach_get('value', fold)
    set_attr(obj, 'gP', co + 2.0)
    set_attr(obj, 'gN', nrm * 0.5 + 0.5)
    set_attr(obj, 'gF', np.stack([fold * 100.0 + 0.5, np.zeros_like(fold), np.zeros_like(fold)], 1))
    _, _, mi, isl, _, _, _ = face_arrays(obj)
    set_attr(obj, 'gI', corner_values(obj, np.stack([isl / 32.0, mi / 4.0, np.zeros(len(isl))], 1)), 'CORNER')
    maps = bake_attrs(obj, size, ['gP', 'gN', 'gF', 'gI'], slot=0)
    valid = maps['valid']
    iy, ix = np.nonzero(valid)
    P = maps['gP'][iy, ix].astype(np.float64) - 2.0
    N = maps['gN'][iy, ix].astype(np.float64) * 2.0 - 1.0
    N /= np.linalg.norm(N, axis=1, keepdims=True) + 1e-9
    fd = (maps['gF'][iy, ix, 0] - 0.5) / 100.0
    I = np.rint(maps['gI'][iy, ix, 0] * 32.0).astype(int)
    x, y, z = P[:, 0], P[:, 1], P[:, 2]
    n_tex = len(x)
    pel, leg = f.pel, f.leg
    left_isl = np.isin(I, [J_FRONT_L, J_BACK_L, J_POCKET_L, J_HEM_FACING_L, J_HEM_FACING_LB])
    right_isl = np.isin(I, [J_FRONT_R, J_BACK_R, J_POCKET_R, J_HEM_FACING_R, J_HEM_FACING_RB])
    sx = np.where(left_isl, 1.0, np.where(right_isl, -1.0, np.where(x >= 0, 1.0, -1.0)))
    # Where each texel is: round the pelvis (psi, mirrored so 0 is the side seam, +pi/2 center back, -pi/2 center front)
    # and round its leg (phi, the same convention).
    pc = pel.center(z)
    psi_raw = np.mod(np.arctan2(y - pc[:, 1], x), 2 * math.pi)
    psim = wrap_pi(np.where(sx > 0, psi_raw, math.pi - psi_raw))
    Rp = pel.radius(z, psi_raw)
    lc = leg.center(z)
    phi = np.arctan2(y - lc[:, 1], sx * x - lc[:, 0])
    Rl = leg.radius(z, phi)
    is_pel = z > f.z_split
    hem_z = np.interp(np.mod(phi, 2 * math.pi), THS, f.hem, period=2 * math.pi)
    zr = z - hem_z
    wtop = waist_top(f, psi_raw)
    dd = (wtop - WAISTBAND[-1]) - z            # depth below the waistband's seam
    d_out = np.where(is_pel, Rp * psim, Rl * phi)   # from the outseam, + toward the back
    d_in = Rl * wrap_pi(phi - math.pi)              # from the inseam, + toward the back
    panel = np.isin(I, [J_FRONT_L, J_BACK_L, J_FRONT_R, J_BACK_R])
    front_panel = np.isin(I, [J_FRONT_L, J_FRONT_R])
    back_panel = np.isin(I, [J_BACK_L, J_BACK_R])

    rng_seed = 5
    fade = np.zeros(n_tex)
    grime = np.zeros(n_tex)
    height = np.zeros(n_tex)
    thread = np.zeros(n_tex)
    rough = np.full(n_tex, 0.86)
    metal = np.zeros(n_tex)
    special = np.zeros((n_tex, 3))   # leather and copper colors
    special_w = np.zeros(n_tex)
    inside = np.isin(I, J_INSIDE)

    def add_stitch(mask, d, s, **kw):
        nonlocal thread, height
        cov, h = stitch_row(d, s, **kw)
        thread = np.maximum(thread, cov * mask)
        height += h * mask

    def add_seam(mask, d, groove=0.0005, width=0.0012):
        nonlocal height
        height -= groove * np.exp(-(d / width) ** 2) * mask

    def raise_band(mask, d0, d1, d, amt, soft=0.0012):
        nonlocal height
        height += amt * smoothstep(d0 - soft, d0 + soft, d) * (1.0 - smoothstep(d1 - soft, d1 + soft, d)) * mask

    # --- the cloth itself: slubby warp streaks, mottling, and the wash.
    Pn = P * [1.0, 1.0, 1.0]
    slub = texmaps.value_noise(np.stack([x / 0.0011, y / 0.0011, z / 0.03], 1) * 0.01, 0.01, seed=rng_seed, octaves=2)
    mott = texmaps.value_noise(Pn, 0.06, seed=rng_seed + 1, octaves=3)
    fine = texmaps.value_noise(Pn, 0.004, seed=rng_seed + 2, octaves=2)
    fade += 0.10 * (slub - 0.5) + 0.12 * (mott - 0.5) + 0.05 * (fine - 0.5)
    front_facing = np.clip(-N[:, 1], 0.0, 1.0)
    back_facing = np.clip(N[:, 1], 0.0, 1.0)
    lg_mask = (~is_pel) & panel
    # Thighs, knees, seat: worn lighter where the cloth rubs and stretches.
    fade += 0.30 * np.exp(-((z - (f.z_knee + 0.2)) / 0.11) ** 2) * front_facing ** 1.5 * panel
    fade += 0.34 * np.exp(-((z - (f.z_knee + 0.015)) / 0.055) ** 2) * front_facing ** 2 * panel
    fade += 0.26 * np.exp(-((z - (f.z_crotch + 0.06)) / 0.06) ** 2) * back_facing ** 1.2 * panel
    fade += 0.12 * np.exp(-((z - (f.z_crotch - 0.02)) / 0.05) ** 2) * np.clip(np.abs(np.cos(phi)) - 0.2, 0, 1) * lg_mask * smoothstep(f.z_split, f.z_split - 0.03, z)
    # Ridges of the folds (the stack, behind the knee) rub light, softly and unevenly; their hollows stay dark.
    rub = texmaps.value_noise(Pn, 0.018, seed=rng_seed + 3, octaves=2)
    ridge = np.clip(fd / 0.005, -0.4, 1.0)
    fade += 0.24 * np.sign(ridge) * np.abs(ridge) ** 1.3 * np.clip(0.45 + 0.8 * (rub - 0.5) + 0.3 * (slub - 0.5), 0.1, 1.0)
    # Whiskers fanning from the crotch toward the hip pockets, and the honeycombs behind the knees ride the folds above.
    rngw = np.random.default_rng(23)
    wmask = front_panel & (z > f.z_crotch - 0.05) & (z < f.z_split + 0.09) & (front_facing > 0.25)
    if wmask.any():
        xs = (sx * x)[wmask]
        zs_ = z[wmask]
        acc = np.zeros(wmask.sum())
        for i in range(4):
            for side_jit in (0,):
                z0 = f.z_crotch + 0.014 + 0.022 * i + rngw.uniform(-0.006, 0.006)
                x0 = 0.03 + rngw.uniform(0.0, 0.015)
                x1 = 0.13 + rngw.uniform(-0.02, 0.02)
                z1 = z0 + 0.025 + 0.012 * i + rngw.uniform(-0.006, 0.006)
                d, along = texmaps.polyline_distance(np.stack([xs, zs_], 1), [(x0, z0), ((x0 + x1) / 2, (z0 + z1) / 2 - 0.004), (x1, z1)])
                w = 0.0028 * (1.0 - 0.6 * along) * np.sin(np.pi * np.clip(along, 0, 1)) ** 0.5 + 0.0004
                acc = np.maximum(acc, (1.0 - smoothstep(0.0, w * 1.6, d)) * np.sin(np.pi * np.clip(along, 0, 1)) ** 0.7 * rngw.uniform(0.6, 1.0))
        fade[wmask] += 0.26 * acc * (0.6 + 0.8 * (fine[wmask] - 0.5) + 0.6 * (mott[wmask] - 0.5))
        height[wmask] -= 0.0003 * acc

    # --- outseam: a plain seam pressed toward the back, its ridge worn pale.
    m = panel & (np.abs(d_out) < 0.02)
    add_seam(m, d_out)
    raise_band(m, 0.0, 0.009, d_out, 0.00045)
    fade += 0.25 * np.exp(-((d_out - 0.004) / 0.003) ** 2) * m
    # --- inseam: flat-felled, two rows of stitching.
    m = lg_mask & (z < f.z_crotch + 0.01) & (np.abs(d_in) < 0.02)
    add_seam(m, d_in)
    raise_band(m, 0.0, 0.0105, d_in, 0.0006)
    add_stitch(m, d_in - 0.0028, z)
    add_stitch(m, d_in - 0.0092, z)
    fade += 0.2 * np.exp(-((d_in - 0.005) / 0.004) ** 2) * m
    # --- the rise: center front below the fly and center back, flat-felled (toward the wearer's left).
    xc = sx * x
    m = panel & (np.abs(x) < 0.02) & ((dd > 0.13) | back_panel) & (z > f.z_crotch - 0.03)
    add_seam(m, np.abs(x))
    raise_band(m & (sx > 0), 0.0, 0.0105, xc, 0.0006)
    add_stitch(m & (sx > 0), xc - 0.0028, z)
    add_stitch(m & (sx > 0), xc - 0.0092, z)
    # --- hem: turned up twice and stitched, its edge roped with wear; grime from the street.
    m = lg_mask | np.isin(I, J_HEMS)
    hm = lg_mask & (zr < 0.03)
    raise_band(hm, -0.01, 0.019, zr, 0.0007, soft=0.0008)
    add_seam(hm, zr - 0.0195, groove=0.0003)
    add_stitch(hm, zr - 0.0145, Rl * phi, period=0.0036)
    rope = 0.5 + 0.5 * np.sin(2 * math.pi * (Rl * phi / 0.016 + zr / 0.012))
    fade += (0.35 * rope + 0.12) * np.exp(-(zr / 0.0035) ** 2) * lg_mask
    grime += 0.45 * (1.0 - smoothstep(0.0, 0.05, zr)) * lg_mask * (0.6 + 0.4 * back_facing)
    grime += 0.25 * (1.0 - smoothstep(0.0, 0.03, zr)) * np.isin(I, J_HEMS)
    # Scuffed where the back of the hem drags on the heel: worn through in soft patches, not speckle.
    heel = np.exp(-(wrap_pi(phi - math.pi / 2) / 0.35) ** 2) * (1.0 - smoothstep(0.0, 0.02, zr)) * lg_mask
    scuff = texmaps.value_noise(Pn, 0.006, seed=rng_seed + 4, octaves=2)
    fade += 0.3 * heel * smoothstep(0.4, 0.7, scuff)

    # --- the waistband: stitched along both edges, its top edge worn.
    wb = np.isin(I, J_BANDS)
    dt = wtop - z
    add_stitch(wb, dt - 0.0028, Rp * psi_raw)
    add_stitch(wb, dt - (WAISTBAND[-1] - 0.0028), Rp * psi_raw)
    fade += 0.3 * np.exp(-(dt / 0.003) ** 2) * wb + 0.18 * np.exp(-((dt - WAISTBAND[-1]) / 0.003) ** 2) * wb
    add_seam(panel & (np.abs(dd) < 0.006), dd, groove=0.0006)
    # Its overlapping end at center front, past the button.
    m = wb & (np.abs(x + 0.006) < 0.01)
    add_seam(m, x + 0.006, groove=0.0008, width=0.0009)
    add_stitch(m & (x > -0.006), x + 0.0035, z, period=0.003)
    # --- the fly: a J of stitching on the wearer's left, the opening's edge at center front, a bar tack at the bottom.
    m = front_panel & (sx > 0) & is_pel & (dd > -0.002) & (dd < 0.16)
    if m.any():
        a_cf = (Rp * (psim + math.pi / 2))[m]
        d2 = dd[m]
        Jr = 0.032
        jpts = [(Jr, 0.0), (Jr, 0.098)] + [(Jr * math.cos(t), 0.098 + Jr * math.sin(t)) for t in np.linspace(0.0, math.pi / 2, 10)[1:]]
        dj, sj = texmaps.polyline_distance(np.stack([a_cf, d2], 1), jpts)
        cov, hh = stitch_row(dj, sj * 0.15)
        thread[m] = np.maximum(thread[m], cov)
        height[m] += hh + 0.0011 * (a_cf < Jr - 0.002) * smoothstep(-0.003, 0.004, a_cf)
        cov2, hh2 = stitch_row(a_cf - 0.0045, d2)
        cov2 *= (d2 < 0.12)
        thread[m] = np.maximum(thread[m], cov2)
        height[m] += hh2 * (d2 < 0.12)
        height[m] -= 0.0009 * np.exp(-(a_cf / 0.0012) ** 2) * (d2 < 0.13)
        tack = (np.abs(a_cf - 0.004) < 0.006) & (np.abs(d2 - 0.133) < 0.0022)
        thread[m] = np.maximum(thread[m], tack * (0.6 + 0.4 * (np.sin(a_cf / 0.0006) > 0)))
        height[m] += 0.0004 * tack
        fade[m] += 0.22 * np.exp(-((a_cf - Jr) / 0.003) ** 2) * (d2 < 0.1) + 0.15 * np.exp(-(a_cf / 0.002) ** 2)

    # --- scoop front pockets, the coin pocket on the right, copper rivets.
    a_side = -d_out
    m = front_panel & is_pel & (dd > -0.003) & (a_side > -0.01) & (a_side < 0.15) & (dd < 0.12)
    sc_a, sc_d = 0.105, 0.078
    q = np.sqrt((a_side / sc_a) ** 2 + (dd / sc_d) ** 2)
    e = (q - 1.0) * 0.085                              # roughly the distance from the opening's edge (+ on the panel)
    facing = m & (q < 1.0)
    edge = m & (np.abs(e) < 0.012)
    height -= 0.0013 * facing * smoothstep(0.0, 0.004, -e)
    height += 0.0005 * edge * np.exp(-((e - 0.0025) / 0.002) ** 2)
    grime += 0.35 * facing * np.exp(-(e / 0.004) ** 2) + 0.55 * m * np.exp(-((e + 0.0011) / 0.0013) ** 2)
    fade += 0.35 * edge * np.exp(-((e - 0.0015) / 0.0018) ** 2)
    add_stitch(m & (q > 0.8), e - 0.0062, np.arctan2(dd, a_side) * 0.09)
    coin = facing & (sx < 0) & (a_side > 0.028) & (a_side < 0.086) & (dd > 0.006)
    cz = 0.012
    add_stitch(coin, a_side - 0.0315, dd)
    add_stitch(coin, a_side - 0.0825, dd)
    add_stitch(coin, dd - cz, a_side)
    add_stitch(coin, dd - (cz + 0.006), a_side)
    height += 0.0007 * coin
    fade += 0.25 * coin * (np.exp(-((a_side - 0.03) / 0.002) ** 2) + np.exp(-((a_side - 0.084) / 0.002) ** 2))
    rivets = []
    for s in (1.0, -1.0):
        rivets += [(s, sc_a + 0.007, 0.006, 0.0042), (s, 0.006, sc_d + 0.007, 0.0042)]
    rivets += [(-1.0, 0.0315, cz + 0.002, 0.0032), (-1.0, 0.0825, cz + 0.002, 0.0032)]
    for s, ra, rd, rr in rivets:
        mm = m & (sx == s) & (np.abs(a_side - ra) < 0.008) & (np.abs(dd - rd) < 0.008)
        if not mm.any():
            continue
        r = np.hypot(a_side[mm] - ra, dd[mm] - rd)
        dome = np.clip(1.0 - (r / rr) ** 2, 0.0, 1.0)
        cover = smoothstep(rr * 1.05, rr * 0.9, r)
        height[mm] += 0.0012 * np.sqrt(dome) + 0.0003 * np.exp(-((r - rr * 1.15) / 0.0006) ** 2)
        patina = 0.35 + 0.65 * smoothstep(rr * 0.2, rr, r)
        special[mm] = mix3(lin(COPPER), lin(0x3a2416), 0.25 * patina)
        special_w[mm] = np.maximum(special_w[mm], cover)
        metal[mm] = np.maximum(metal[mm], cover * (0.9 - 0.3 * patina))
        rough[mm] = rough[mm] * (1 - cover) + cover * (0.32 + 0.2 * patina)

    # --- the back: yoke (flat-felled), seat.
    zy = wtop - WAISTBAND[-1] - yoke_depth(psi_raw)
    d_y = z - zy
    m = back_panel & is_pel & (np.abs(d_y) < 0.02)
    add_seam(m, d_y)
    raise_band(m, -0.0105, 0.0, d_y, 0.0006)
    add_stitch(m, d_y + 0.0028, Rp * psi_raw)
    add_stitch(m, d_y + 0.0092, Rp * psi_raw)
    fade += 0.2 * np.exp(-((d_y + 0.005) / 0.004) ** 2) * m

    # --- leather patch on the back waistband, the wearer's right.
    art = patch_artwork()
    pc_psi = 0.5 * math.pi + 0.3
    pa = (Rp * (psi_raw - pc_psi))
    pb = (wtop - WAISTBAND[-1] / 2) - z
    pw, ph = 0.065, 0.031
    m = wb & (np.abs(pa) < pw / 2 + 0.003) & (np.abs(pb) < ph / 2 + 0.003)
    if m.any():
        u = pa[m] / pw + 0.5
        v = -pb[m] / ph + 0.5
        cover = smoothstep(0.0, 0.004, np.minimum(np.minimum(u, 1 - u) * pw, np.minimum(v, 1 - v) * ph))
        deb = sample_img(art, u, v)
        age = texmaps.value_noise(P[m], 0.012, seed=9, octaves=3)
        col = mix3(lin(LEATHER), lin(0x5b3a20), 0.25 + 0.5 * deb + 0.3 * (age - 0.5))
        special[m] = col
        special_w[m] = np.maximum(special_w[m], cover)
        rough[m] = rough[m] * (1 - cover) + cover * (0.58 + 0.15 * age)
        height[m] += cover * (0.0012 - 0.0005 * deb)
        su = np.minimum(np.minimum(u, 1 - u) * pw, np.minimum(v, 1 - v) * ph)
        cov, hh = stitch_row(su - 0.0016, np.where(np.minimum(u, 1 - u) * pw < np.minimum(v, 1 - v) * ph, v * ph, u * pw))
        thread[m] = np.maximum(thread[m], cov * cover)
        height[m] += hh * cover

    # --- belt loops: stitched down both edges, bar-tacked at the ends.
    m = I == J_LOOP
    if m.any():
        loops = np.array(f.loops)
        dpsi = wrap_pi(psi_raw[m][:, None] - loops[None, :])
        k = np.argmin(np.abs(dpsi), axis=1)
        lat = Rp[m] * dpsi[np.arange(m.sum()), k]
        zz = z[m]
        cov1, h1 = stitch_row(np.abs(lat) - 0.0037, zz, width=0.0008, period=0.003)
        bot = wtop[m] - WAISTBAND[-1] - 0.009
        tack = (zz < bot + 0.006) * (np.abs(lat) < 0.004)
        thread[m] = np.maximum(thread[m], np.maximum(cov1, tack * (0.5 + 0.5 * (np.sin(lat / 0.0005) > 0))))
        height[m] += h1 + 0.0003 * tack
        fade[m] += 0.3 * np.exp(-((np.abs(lat) - 0.0052) / 0.0012) ** 2)

    # --- the back pockets: double-stitched edges, a folded top (plain: no decorative stitching), a wallet worn into the right one.
    for pk in f.pockets:
        m = I == (J_POCKET_L if pk['side'] == 0 else J_POCKET_R)
        if not m.any():
            continue
        sg = 1.0 if pk['side'] == 0 else -1.0
        a = -sg * wrap_pi(psi_raw[m] - pk['psi']) * pk['R']
        b = pk['z_top'] - z[m]
        w2 = pk['w'] / 2
        s_ = np.clip(a / w2, -1.0, 1.0)
        Hs = pk['hs'] + (pk['hp'] - pk['hs']) * (1.0 - np.abs(s_))
        side_d = w2 * (1.0 - 0.05 * np.clip(b / Hs, 0, 1)) - np.abs(a + 0.009 * np.clip(b / Hs, 0, 1) ** 2)
        bot_d = (Hs - b) * 0.75
        ed = np.minimum(side_d, bot_d)                 # distance in from the side and bottom edges
        along = np.where(side_d < bot_d, b, a)
        for off in (0.0025, 0.0085):
            cov, hh = stitch_row((ed - off) * (b > 0.02), along)
            thread[m] = np.maximum(thread[m], cov * (b > 0.019))
            height[m] += hh * (b > 0.019)
        cov, hh = stitch_row(b - 0.017, a)
        thread[m] = np.maximum(thread[m], cov)
        height[m] += hh + 0.0005 * (b < 0.019)
        height[m] -= 0.0004 * np.exp(-((b - 0.0195) / 0.0012) ** 2)
        # No decorative stitching on the pocket (every maker's design is its own mark): plain patches, only the cloth
        # puckering a little along the double row where the stitching pulls it.
        height[m] -= 0.00025 * np.exp(-((ed - 0.0055) / 0.003) ** 2) * (b > 0.02)
        fade[m] += 0.32 * np.exp(-(ed / 0.0022) ** 2) + 0.2 * np.exp(-(b / 0.003) ** 2)
        if pk['side'] == 1:
            wa, wb_ = 0.046, 0.058
            ring = np.maximum(np.abs(a - 0.004) - wa, np.abs(b - 0.085) - wb_)
            fade[m] += 0.22 * np.exp(-(ring / 0.004) ** 2) + 0.1 * (ring < 0)
        tack = ((np.abs(a - w2 + 0.005) < 0.004) | (np.abs(a + w2 - 0.005) < 0.004)) & (b < 0.012)
        thread[m] = np.maximum(thread[m], tack * 0.8)
        height[m] += 0.0003 * tack

    # --- the inside (facings): the denim's pale weft side.
    fade = np.clip(fade, -0.15, 1.0)
    base = mix3(lin(INDIGO), lin(INDIGO_WORN), np.clip(fade, 0.0, 1.0)) * (1.0 + 0.6 * np.clip(fade, -0.15, 0.0))[:, None]
    base = np.where(inside[:, None], lin(DENIM_BACK) * (0.85 + 0.3 * (fine - 0.5))[:, None], base)
    base = mix3(base, base * lin(0x8a7a64) * 1.4, np.clip(grime, 0, 0.8))
    thread_col = mix3(lin(THREAD), lin(0xd8c39a), np.clip(fade, 0, 1) * 0.6) * (0.85 + 0.3 * fine)[:, None]
    base = mix3(base, thread_col, thread)
    rough = rough * (1 - thread) + 0.72 * thread
    base = mix3(base, special, special_w)
    rough += 0.04 * (fine - 0.5) - 0.06 * grime
    print(f'[garments] {name}: denim painted ({n_tex} texels, {time.time() - t0:.0f}s)')
    return {'iy': iy, 'ix': ix, 'P': P, 'valid': valid, 'base': base, 'rough': rough, 'metal': metal, 'height': height}


def texture_material(name, maps, size, specular=0.3, sheen=0.0):
    """A Principled material from painted maps (linear base color, roughness, metallic, height), via image files."""
    iy, ix, valid = maps['iy'], maps['ix'], maps['valid']

    def full(vals, ch):
        img = np.zeros((size, size, ch))
        img[iy, ix] = vals.reshape(len(iy), ch)
        return texmaps.dilate(img, valid, steps=6)

    base = full(maps['base'], 3)
    orm = np.zeros((size, size, 3))
    orm[iy, ix, 1] = maps['rough']
    orm[iy, ix, 2] = maps['metal']
    orm = texmaps.dilate(orm, valid, steps=6)
    hgt = np.zeros((size, size))
    hgt[iy, ix] = maps['height']
    Pimg = np.zeros((size, size, 3))
    Pimg[iy, ix] = maps['P']
    nrm = texmaps.height_to_normal(hgt, Pimg, valid)
    nrm = texmaps.dilate(nrm, valid, steps=6)
    p_base = texmaps.save(f'{name}_paint_base', base)
    p_orm = texmaps.save(f'{name}_paint_rm', orm, 'Non-Color')
    p_nrm = texmaps.save(f'{name}_paint_normal', nrm * 0.5 + 0.5, 'Non-Color')
    m = core.Mat(f'{name}_paint')
    b = m.image(p_base)
    m.link(b.outputs['Color'], m.bsdf.inputs['Base Color'])
    r = m.image(p_orm, non_color=True)
    sep = m.node('ShaderNodeSeparateColor')
    m.link(r.outputs['Color'], sep.inputs['Color'])
    m.link(sep.outputs['Green'], m.bsdf.inputs['Roughness'])
    m.link(sep.outputs['Blue'], m.bsdf.inputs['Metallic'])
    nn = m.image(p_nrm, non_color=True)
    nm = m.node('ShaderNodeNormalMap')
    m.link(nn.outputs['Color'], nm.inputs['Color'])
    m.link(nm.outputs['Normal'], m.bsdf.inputs['Normal'])
    m.set("Specular IOR Level", specular)
    if sheen > 0:
        m.set('Sheen Weight', sheen)
    return m.m


def metal_material(name, hex_color, rough=0.3):
    m = core.Mat(name)
    tc = m.node('ShaderNodeTexCoord')
    nz = m.node('ShaderNodeTexNoise', Scale=900.0, Detail=4.0)
    m.link(tc.outputs['Object'], nz.inputs['Vector'])
    m.set('Base Color', m.mix(nz.outputs['Fac'], core.hex_linear(hex_color), core.hex_linear(0x3b2a1c)))
    m.set('Metallic', 1.0)
    m.set('Roughness', rough)
    return m.m



# ------------------------------------------------------------------ skinning

def group_weights(obj, bone_index):
    """Dense (vertices x bones) weights of a mesh's vertex groups, by the armature's bone order."""
    W = np.zeros((len(obj.data.vertices), len(bone_index)), dtype=np.float32)
    names = {g.index: g.name for g in obj.vertex_groups}
    for v in obj.data.vertices:
        for g in v.groups:
            b = bone_index.get(names[g.group])
            if b is not None:
                W[v.index, b] = g.weight
    return W


def allowed_bones(names, prefixes, exclude=()):
    keep = np.array([any(n.startswith(p) for p in prefixes) and not any(e in n for e in exclude) for n in names])
    return keep


def transfer_weights(h, garment, sources, keep, smooth_iter=12, max_influences=8, extra_smooth=None):
    """Skin weights for a garment from the nearest point of the body and its outfit (barycentric), kept to the bones
    the garment should follow, smoothed over the garment's own edges, at most max_influences per vertex."""
    names = h.bone_names
    bidx = {n: i for i, n in enumerate(names)}
    Vs, Fs, Ws = [], [], []
    off = 0
    for obj, tri_mask in sources:
        V, F = tri_mesh(obj)
        W = group_weights(obj, bidx)
        if tri_mask is not None:
            F = F[tri_mask(V, F)]
        Vs.append(V)
        Fs.append(F + off)
        Ws.append(W)
        off += len(V)
    V = np.concatenate(Vs)
    F = np.concatenate(Fs)
    W = np.concatenate(Ws)
    W[:, ~keep] = 0.0
    tree = BVHTree.FromPolygons([tuple(p) for p in V], [tuple(t) for t in F])
    G = get_co(garment) if garment.parent is None else np.array([garment.matrix_world @ v.co for v in garment.data.vertices])
    out = np.zeros((len(G), len(names)), dtype=np.float32)
    for i, p in enumerate(G):
        loc, nor, fi, dist = tree.find_nearest(Vector(p))
        a, b, c = F[fi]
        pa, pb, pc = V[a], V[b], V[c]
        bc = barycentric(np.array(loc), pa, pb, pc)
        out[i] = bc[0] * W[a] + bc[1] * W[b] + bc[2] * W[c]
    s = out.sum(1, keepdims=True)
    out = np.where(s > 1e-6, out / np.maximum(s, 1e-6), out)
    # Smooth over the garment's edges (the weights may jump where the nearest body part changes).
    me = garment.data
    ev = np.empty(len(me.edges) * 2, dtype=np.int64)
    me.edges.foreach_get('vertices', ev)
    ev = ev.reshape(-1, 2)
    deg = np.bincount(ev.ravel(), minlength=len(G)).astype(np.float32)
    it = np.full(len(G), smooth_iter) if extra_smooth is None else smooth_iter + extra_smooth(G)
    for k in range(int(it.max())):
        acc = np.zeros_like(out)
        np.add.at(acc, ev[:, 0], out[ev[:, 1]])
        np.add.at(acc, ev[:, 1], out[ev[:, 0]])
        avg = acc / np.maximum(deg, 1)[:, None]
        act = (k < it)[:, None] & (deg > 0)[:, None]
        out = np.where(act, 0.5 * out + 0.5 * avg, out)
    # Keep the strongest influences.
    if out.shape[1] > max_influences:
        thr = -np.sort(-out, axis=1)[:, max_influences - 1:max_influences]
        out = np.where(out >= thr, out, 0.0)
    out[out < 0.004] = 0.0
    out /= np.maximum(out.sum(1, keepdims=True), 1e-6)
    return out


def barycentric(p, a, b, c):
    v0, v1, v2 = b - a, c - a, p - a
    d00, d01, d11 = v0 @ v0, v0 @ v1, v1 @ v1
    d20, d21 = v2 @ v0, v2 @ v1
    den = d00 * d11 - d01 * d01
    if abs(den) < 1e-18:
        return np.array([1.0, 0.0, 0.0])
    v = (d11 * d20 - d01 * d21) / den
    w = (d00 * d21 - d01 * d20) / den
    bc = np.clip(np.array([1.0 - v - w, v, w]), 0.0, 1.0)
    return bc / bc.sum()


def bind(h, garment, W):
    """Parents the garment to the hero's armature as the body is (its data in the armature's space, centimeters),
    with vertex groups for the bones it uses and an Armature modifier."""
    inv = h.arm.matrix_world.inverted()
    garment.data.transform(inv)
    garment.parent = h.arm
    garment.matrix_parent_inverse = Matrix.Identity(4)
    garment.matrix_basis = Matrix.Identity(4)
    for b in np.nonzero(W.max(0) > 0)[0]:
        vg = garment.vertex_groups.new(name=h.bone_names[b])
        idx = np.nonzero(W[:, b] > 0)[0]
        for i in idx:
            vg.add([int(i)], float(W[i, b]), 'REPLACE')
    mod = garment.modifiers.new('Armature', 'ARMATURE')
    mod.object = h.arm
    bpy.context.view_layer.update()


JEANS_BONES = ('pelvis', 'spine_01', 'spine_02', 'thigh_', 'calf_')
JACKET_BONES = ('pelvis', 'spine_', 'neck_01', 'neck_02', 'clavicle_', 'upperarm_', 'lowerarm_', 'thigh_')


def skin_jeans(h, jeans, f):
    keep = allowed_bones(h.bone_names, JEANS_BONES)
    ankle = float(h.bone('foot_l')[2]) + 0.03
    sources = [(h.body, lambda V, F: V[F][:, :, 2].min(1) > ankle), (h.outfit, lambda V, F: ~h.o_shirt)]
    # The hem's stack and the cloth below the knee hang loose: smoothed more.
    W = transfer_weights(h, jeans, sources, keep, smooth_iter=4,
                         extra_smooth=lambda G: (20 * (1.0 - smoothstep(0.1, 0.3, G[:, 2]))).astype(int))

    # (Fitting the weights to example poses, so each vertex keeps its offset from the skin, was tried: it lowers the average
    # miss but not the knee in a 90-degree sit, where the body's own skin folds in; these keep the body's knee correctives.)
    bind(h, jeans, W)
    return W


# ------------------------------------------------------------------ export

def export_fbx(h, garment, path):
    """The garment and the hero's whole armature (every bone, as the body has them) as an FBX for Unreal: centimeters,
    the armature node named root (Unreal reads it as the root bone), no leaf bones, rest pose as the bind pose."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    saved = {pb.name: (pb.location.copy(), pb.rotation_quaternion.copy(), pb.scale.copy()) for pb in h.arm.pose.bones}
    for pb in h.arm.pose.bones:
        pb.location = (0, 0, 0)
        pb.rotation_quaternion = (1, 0, 0, 0)
        pb.rotation_euler = (0, 0, 0)
        pb.scale = (1, 1, 1)
    bpy.context.view_layer.update()
    core.select_only([garment, h.arm], h.arm)
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={'ARMATURE', 'MESH'}, use_mesh_modifiers=False,
                             mesh_smooth_type='FACE', add_leaf_bones=False, primary_bone_axis='Y', secondary_bone_axis='X',
                             use_armature_deform_only=False, armature_nodetype='NULL', bake_anim=False, apply_unit_scale=True,
                             apply_scale_options='FBX_SCALE_NONE', global_scale=1.0, axis_forward='Y', axis_up='Z',
                             use_tspace=False, path_mode='STRIP', embed_textures=False, use_custom_props=False)
    for pb in h.arm.pose.bones:
        loc, rot, sc = saved[pb.name]
        pb.location, pb.rotation_quaternion, pb.scale = loc, rot, sc
    bpy.context.view_layer.update()
    return path


def copy_textures(name, slots):
    """The baked maps next to the FBX (unreal/Art/Garments/Textures), where garments_setup imports them."""
    import shutil
    src = os.path.join(core.BUILD_DIR, 'textures', name)
    dst = os.path.join(GARMENT_DIR, 'Textures')
    os.makedirs(dst, exist_ok=True)
    out = []
    for f in sorted(os.listdir(src)):
        if f.endswith('.png') and f.startswith(f'T_{name}_'):
            shutil.copyfile(os.path.join(src, f), os.path.join(dst, f))
            out.append(f)
    return out


# ------------------------------------------------------------------ poses for review

def pose_reset(h):
    for pb in h.arm.pose.bones:
        pb.rotation_mode = 'QUATERNION'
        pb.location = (0, 0, 0)
        pb.rotation_quaternion = (1, 0, 0, 0)
        pb.scale = (1, 1, 1)
    hide_toes(h)
    bpy.context.view_layer.update()


def turn(h, name, axis, deg):
    """Turns a bone (and what hangs from it) about a world axis through its head, as the game's procedural
    animation does (UStreetBodyAnim rotates the main bones in component space)."""
    pb = h.arm.pose.bones.get(name)
    if pb is None:
        return
    bpy.context.view_layer.update()
    M = pb.matrix.copy()
    head = M.translation.copy()
    R = Matrix.Rotation(math.radians(deg), 4, Vector(axis))
    pb.matrix = Matrix.Translation(head) @ R @ Matrix.Translation(-head) @ M
    bpy.context.view_layer.update()


X, Y, Z = (1, 0, 0), (0, 1, 0), (0, 0, 1)


def pose(h, kind):
    """Review poses: the game's stance (arms hanging), a walking stride, seated, arms raised. Angles about world axes:
    +X pitches a limb back (toward +Y), -X forward."""
    pose_reset(h)
    if kind == 'bind':
        return
    if kind in ('stand', 'walk', 'sit'):
        turn(h, 'upperarm_l', Y, 31)
        turn(h, 'upperarm_r', Y, -31)
        turn(h, 'lowerarm_l', X, -8)
        turn(h, 'lowerarm_r', X, -8)
    if kind == 'walk':
        turn(h, 'pelvis', Z, 5)
        turn(h, 'thigh_l', X, -26)
        turn(h, 'calf_l', X, 8)
        turn(h, 'foot_l', X, -10)
        turn(h, 'thigh_r', X, 18)
        turn(h, 'calf_r', X, 38)
        turn(h, 'foot_r', X, 12)
        turn(h, 'spine_03', Z, -6)
        turn(h, 'upperarm_l', X, 18)
        turn(h, 'upperarm_r', X, -20)
        turn(h, 'lowerarm_r', X, -22)
    if kind == 'sit':
        turn(h, 'thigh_l', X, -88)
        turn(h, 'thigh_r', X, -88)
        turn(h, 'calf_l', X, 92)
        turn(h, 'calf_r', X, 92)
        turn(h, 'spine_01', X, -6)
        turn(h, 'upperarm_l', X, -35)
        turn(h, 'upperarm_r', X, -35)
        turn(h, 'lowerarm_l', X, -55)
        turn(h, 'lowerarm_r', X, -55)
    if kind == 'reach':
        turn(h, 'upperarm_l', Y, -105)
        turn(h, 'upperarm_r', Y, 105)
        turn(h, 'lowerarm_l', Y, -15)
        turn(h, 'lowerarm_r', Y, 15)
        turn(h, 'spine_03', X, 4)


def pokes(h, garment, region, tol=0.002, skip=(), source=None):
    """Vertices of a mesh (the body, or the jeans under the jacket) that come out through a garment in the current pose:
    (count, worst depth in m, how many were tested). region(rest positions) picks the vertices the garment covers."""
    src = source or h.body
    dg = bpy.context.evaluated_depsgraph_get()
    rest = np.array([src.matrix_world @ v.co for v in src.data.vertices])
    sel = np.nonzero(region(rest))[0]
    se = src.evaluated_get(dg)
    sm = se.to_mesh()
    mw = src.matrix_world
    pts = [mw @ sm.vertices[int(i)].co for i in sel]
    se.to_mesh_clear()
    ge = garment.evaluated_get(dg)
    gm = ge.to_mesh()
    gmw = garment.matrix_world
    verts = [gmw @ v.co for v in gm.vertices]
    isl = np.empty(len(garment.data.polygons), dtype=np.int64)
    garment.data.attributes["island"].data.foreach_get("value", isl)
    polys = [tuple(p.vertices) for p in gm.polygons if isl[p.index] not in skip]
    ge.to_mesh_clear()
    tree = BVHTree.FromPolygons(verts, polys)
    count, worst = 0, 0.0
    for p in pts:
        loc, nor, idx, dist = tree.find_nearest(p)
        if loc is None:
            continue
        d = (p - loc).dot(nor)
        if d > tol:
            count += 1
            worst = max(worst, d)
    return count, worst, len(sel)



# ------------------------------------------------------------------ the jacket

JK_M = 112                                       # columns round the torso: 0 at center front, rising toward the wearer's left
BAND = (0.0, 0.004, 0.011, 0.019, 0.027, 0.035, 0.043, 0.051, 0.056)   # hem band rows above its bottom edge (last: its seam)
SHELL, RIB, HARDWARE = 0, 1, 2
(K_FRONT_L, K_BACK, K_FRONT_R, K_SLEEVE_LF, K_SLEEVE_LB, K_SLEEVE_RF, K_SLEEVE_RB, K_BAND_L, K_BAND_B, K_BAND_R,
 K_BANDIN_L, K_BANDIN_B, K_BANDIN_R, K_COLLAR_L, K_COLLAR_R, K_COLLARIN_L, K_COLLARIN_R, K_CUFF_LF, K_CUFF_LB, K_CUFF_RF,
 K_CUFF_RB, K_CUFFIN_L, K_CUFFIN_R, K_ZIP, K_WELT_L, K_WELT_R, K_PULL) = range(27)
K_INSIDE = (K_BANDIN_L, K_BANDIN_B, K_BANDIN_R, K_COLLARIN_L, K_COLLARIN_R, K_CUFFIN_L, K_CUFFIN_R)


def tee_parts(h):
    """The tee's triangles sorted out: its sleeves (left, right), its collar band, the rest (front, back, hem)."""
    sh = h.o_shirt
    isl = h.oIsl
    left, right, collar = np.zeros(len(sh), bool), np.zeros(len(sh), bool), np.zeros(len(sh), bool)
    for i in np.unique(isl[sh]):
        m = sh & (isl == i)
        V = h.oV[np.unique(h.oF[m])]
        if V[:, 0].min() > 0.12:
            left |= m
        elif V[:, 0].max() < -0.12:
            right |= m
        elif V[:, 2].min() > 1.38 and np.abs(V[:, 0]).max() < 0.14:
            collar |= m
    return left, right, collar, sh & ~left & ~right


def jacket_fit(h, jf, jeans):
    k = Fit()
    left, right, collar, torso = tee_parts(h)
    k.tee_left, k.tee_right, k.tee_collar, k.tee_torso = left, right, collar, torso
    # The sleeve's seam round the armhole: the tee sleeve's inner edge.
    SV = h.oV[np.unique(h.oF[left])]
    edge = SV[SV[:, 0] < SV[:, 0].min() + 0.015]
    k.x_ah = float(SV[:, 0].min())
    CV = h.oV[np.unique(h.oF[collar])]
    k.z_neck_min = float(CV[:, 2].min())
    k.z_a = float(edge[:, 2].min()) - 0.02
    # The armhole's top is the shoulder point, just under where the shoulders rise to the neck.
    k.z_S = k.z_neck_min - 0.025
    k.edge = edge
    k.collar_pts = CV
    # The hem: well below the jeans' waistband, so it stays over it when the arms go up.
    k.z_hem = max(jf.z_wb_f - 0.075, float(h.bone("thigh_l")[2]) - 0.035)   # and clear of the thighs when seated
    k.z_b = k.z_hem + BAND[-1]
    k.jeans_top = jf.z_wb_b + 0.01
    k.jeans_front = jf.z_wb_f
    half_w = float(np.abs(CV[:, 0]).max())
    k.neck_w = min(max(0.6 * half_w + 0.02, 0.058), 0.075)
    k.neck_d = 0.92 * k.neck_w
    k.neck_c = np.array([0.0, float(h.bone('neck_01')[1]) - 0.004])
    k.seam_r, k.seam_z = neck_seam(k)
    return k


def torso_table(h, k, jeans):
    """The shell round the torso by height: over the tee (standing in for the chest, back and belly), snug over the
    jeans at the hem band, bloused just above it, easing in toward the neck."""
    JV, JF = tri_mesh(jeans)
    zs = np.arange(k.z_hem - 0.01, k.z_neck_min + 0.09, 0.005)
    tsec = z_sections(h.oV, h.oF[k.tee_torso | k.tee_collar], zs)
    jsec = z_sections(JV, JF, zs)
    hulls = []
    for z, pt, pj in zip(zs, tsec, jsec):
        pts = pj if z < min(k.z_b + 0.02, k.jeans_front - 0.008) else np.concatenate([pt, pj]) if z < k.jeans_top else pt
        if len(pts) < 6:
            hulls.append(None)
            continue
        q = pts[:, :2]
        hulls.append(hull2d(np.concatenate([q, q * [-1.0, 1.0]])))
    hulls = fill_levels(hulls)
    yc = smooth1d(np.array([(H[:, 1].min() + H[:, 1].max()) / 2 for H in hulls]), 4)
    C = np.stack([np.zeros_like(yc), yc], 1)
    ease = np.where(zs < k.z_b, 0.006, 0.006 + 0.016 * smoothstep(k.z_b, k.z_b + 0.025, zs))
    ease = ease + 0.012 * np.exp(-((zs - (k.z_b + 0.04)) / 0.035) ** 2) - 0.012 * smoothstep(k.z_S - 0.02, k.z_neck_min + 0.02, zs)
    R = np.array([radial(H, c, THS) for H, c in zip(hulls, C)])
    R = smooth1d(smooth1d(R, 2, 0), 3, 1, wrap=True) + ease[:, None]
    shell = zs >= k.z_b + 0.01
    R[shell] = span_fill(R[shell], 0.005, 0.5)
    R = smooth1d(R, 1.5, 0)
    return Table(zs, C, R)


def armhole_columns(h, k, tor):
    """Columns bounding the left armhole (the right mirrors it), from where the tee's sleeve is sewn on."""
    d = 2 * math.pi / JK_M
    e = k.edge
    c = tor.center(e[:, 2])
    ang = np.arctan2(e[:, 1] - c[:, 1], e[:, 0])
    q = JK_M // 4
    return q + int(math.floor(ang.min() / d)), q + int(math.ceil(ang.max() / d))


def column_angles(k, Z, k1, k2):
    """Each torso vertex's angle (rows x columns). The columns are even except round the armholes, which they shape into
    ovals: the hole's columns span less angle toward the shoulder (a narrow top where the sleeve's cap sits) and a little
    less than the tee's at the armpit, the panels' columns stretching to make up."""
    M = JK_M
    d = 2 * math.pi / M
    base = -0.5 * math.pi + np.arange(M) * d
    out = np.broadcast_to(base, Z.shape).copy()
    hc = -0.5 * math.pi + 0.5 * (k1 + k2) * d
    hw0 = 0.5 * (k2 - k1) * d
    q = M // 2
    for r in range(len(Z)):
        z = float(Z[r, k1])
        v = min(max((z - k.z_a) / (k.z_S - k.z_a), 0.0), 1.0)
        s = float(smoothstep(k.z_a - 0.05, k.z_a, z) * (1.0 - smoothstep(k.z_S + 0.005, k.z_S + 0.045, z)))
        if s <= 0.0:
            continue
        hw = hw0 * (0.8 - 0.5 * v ** 1.6)
        new = base.copy()
        cols = np.arange(k1, k2 + 1)
        new[k1:k2 + 1] = hc + (cols - 0.5 * (k1 + k2)) / (0.5 * (k2 - k1)) * hw
        front = np.arange(0, k1 + 1)
        new[:k1 + 1] = base[0] + (hc - hw - base[0]) * front / k1
        back = np.arange(k2, q + 1)
        new[k2:q + 1] = (hc + hw) + (base[q] - (hc + hw)) * (back - k2) / (q - k2)
        # The right side mirrors the left: column M - c at pi - angle.
        for c in range(1, q):
            new[M - c] = math.pi - new[c]
        out[r] = base + s * (new - base)
    return out


def neckline(k, psi):
    """The neckline's height round the neck: the tee's collar band's lower edge, a little lower."""
    P = k.collar_pts
    a = np.arctan2(P[:, 1] - k.neck_c[1], P[:, 0])
    out = np.zeros(len(psi))
    for i, p in enumerate(psi):
        dd = np.abs(wrap_pi(a - p))
        sel = dd < 0.25
        out[i] = P[sel, 2].min() if sel.any() else k.z_neck_min
    return smooth1d(out, 3, wrap=True) - 0.004


def torso_rows(k, psi):
    """Each row's height round the torso (rows x columns): the band, the shell to the armpit, the armhole up to its top
    (the shoulders over it to the neck are shoulder_rows'). Returns the rows and the indices of the armpit and armhole-top
    rows, and the old neckline (the tee collar's lower edge) for reference."""
    rows = [np.full(len(psi), k.z_hem + d) for d in BAND]
    n1 = max(4, int(round((k.z_a - k.z_b) / 0.011)))
    for u in np.linspace(0.0, 1.0, n1 + 1)[1:]:
        rows.append(np.full(len(psi), k.z_b + (k.z_a - k.z_b) * u))
    ja = len(rows) - 1
    n2 = max(4, int(round((k.z_S - k.z_a) / 0.0115)))
    for u in np.linspace(0.0, 1.0, n2 + 1)[1:]:
        rows.append(np.full(len(psi), k.z_a + (k.z_S - k.z_a) * u))
    jS = len(rows) - 1
    zn = neckline(k, psi)
    return np.array(rows), ja, jS, zn


def neck_seam(k):
    """Where the collar is sewn on, round the base of the neck: just outside the tee's own collar band at its inner edge
    (which lies on the neck), by angle round the neck's center (THS). Returns the seam's radius and height there."""
    P = k.collar_pts
    c = k.neck_c
    a = np.arctan2(P[:, 1] - c[1], P[:, 0])
    r = np.hypot(P[:, 0], P[:, 1] - c[1])
    ok = r > 0.5 * np.median(r)                      # not the odd vertex near the axis (a label, the tape's end)
    r_in = np.full(NTH, np.nan)
    z_top = np.full(NTH, np.nan)
    for i, th in enumerate(THS):
        sel = ok & (np.abs(wrap_pi(a - th)) < 0.15)
        if sel.sum() < 3:
            continue
        r_in[i] = np.percentile(r[sel], 5)
        near = sel & (r < r_in[i] + 0.012)
        z_top[i] = P[near, 2].max()
    good = ~np.isnan(r_in)
    r_in = np.interp(THS, THS[good], r_in[good], period=2 * math.pi)
    z_top = np.interp(THS, THS[good], z_top[good], period=2 * math.pi)
    # Symmetric (the tee isn't quite), smooth round the neck.
    mirror = (NTH // 2 - np.arange(NTH)) % NTH      # the angle pi - th
    r_in = smooth1d(0.5 * (r_in + r_in[mirror]), 8, wrap=True)
    z_top = smooth1d(0.5 * (z_top + z_top[mirror]), 8, wrap=True)
    return r_in + 0.0045, z_top + 0.004


def shoulder_rows(h, k, PS, n=10):
    """The shell over the shoulders, from the armhole's top row (PS, columns x 3) to the neck seam: each column swept from
    its point on that row toward the seam (its angle round the neck easing to an even spread there), laid on the tee by
    casting out to it from inside the body, its ease running from what it has at the armhole's top down to 8 mm over the
    shoulder and 5 mm at the seam. Returns n rows (n x columns x 3), the last the seam."""
    M = len(PS)
    c = np.array([0.0, k.neck_c[1]])
    r_seam, z_seam = k.seam_r, k.seam_z
    tee = k.tee_torso | k.tee_collar
    TV, TF = h.oV, h.oF[tee]
    tree = BVHTree.FromPolygons([tuple(p) for p in TV], [tuple(t) for t in TF])
    aS = np.arctan2(PS[:, 1] - c[1], PS[:, 0])
    rS = np.hypot(PS[:, 0], PS[:, 1] - c[1])
    a_even = -0.5 * math.pi + np.arange(M) * 2 * math.pi / M
    aE = aS + wrap_pi(a_even - aS)
    th_e = np.mod(aE, 2 * math.pi)
    rE = np.interp(th_e, THS, r_seam, period=2 * math.pi)
    zE = np.interp(th_e, THS, z_seam, period=2 * math.pi)
    # Inside the body under each column's seam point, the casts' origin.
    O = np.stack([c[0] + 0.45 * rE * np.cos(aE), c[1] + 0.45 * rE * np.sin(aE), zE - 0.12], 1)

    def cast(o, p):
        d = p - o
        L = float(np.linalg.norm(d))
        d = d / L
        hit = tree.ray_cast(Vector(o), Vector(d), 0.6)
        return d, L, (hit[3] if hit[0] is not None else None)

    clr0 = np.full(M, 0.008)
    for i in range(M):
        _, L, dist = cast(O[i], PS[i])
        if dist is not None:
            clr0[i] = max(L - dist, 0.004)
    t = np.linspace(0.0, 1.0, n + 1)[1:]
    out = np.zeros((n, M, 3))
    for j, tj in enumerate(t):
        s = float(smoothstep(0.0, 1.0, tj))
        a = aS + wrap_pi(aE - aS) * s
        r = rS + (rE - rS) * tj
        z = PS[:, 2] + (zE - PS[:, 2]) * tj
        guide = np.stack([c[0] + r * np.cos(a), c[1] + r * np.sin(a), z], 1)
        clr = clr0 + (0.008 - clr0) * smoothstep(0.0, 0.5, tj)
        clr = clr + (0.005 - clr) * smoothstep(0.6, 1.0, tj)
        for i in range(M):
            if j == n - 1:
                out[j, i] = guide[i]
                continue
            d, L, dist = cast(O[i], guide[i])
            out[j, i] = O[i] + d * (dist + clr[i]) if dist is not None else guide[i]
    # Even out the rows (the seam row and the armhole's top stay), then keep them off the tee.
    for _ in range(3):
        full = np.concatenate([PS[None], out])
        avg = (np.roll(full[1:-1], 1, 1) + np.roll(full[1:-1], -1, 1)) * 0.25 + (full[:-2] + full[2:]) * 0.25
        out[:-1] = 0.5 * out[:-1] + 0.5 * avg
    for j in range(n - 1):
        for i in range(M):
            loc, nor, _, dist = tree.find_nearest(Vector(out[j, i]))
            if loc is None:
                continue
            nor = np.array(nor)
            radial = np.array([out[j, i][0] - c[0], out[j, i][1] - c[1], 0.0])
            radial /= np.linalg.norm(radial) + 1e-9
            if nor @ radial + nor[2] * 0.5 < 0:
                nor = -nor
            depth = (out[j, i] - np.array(loc)) @ nor
            if depth < 0.0045:
                out[j, i] = out[j, i] + nor * (0.0045 - depth)
    return out


def torso_folds(k, P, psi, rng):
    """Sculpted on the torso: gathers into the hem band, a soft bag of cloth above it at the back, drape under the arms."""
    z = P[..., 2]
    d = np.zeros(z.shape)
    zb = k.z_b
    # Gathers: the shell's fullness taken into the band, dying out over a hand's breadth.
    ph = np.zeros(z.shape)
    for f_, a_, s_ in ((31.0, 1.0, 0.0), (47.0, 0.45, 1.7), (13.0, 0.5, 0.6)):
        ph = ph + a_ * np.sin(f_ * psi + s_ + 0.004 * f_ * np.sin(3 * psi))
    d += 0.0026 * ph * np.exp(-((z - zb) / 0.035) ** 2) * smoothstep(zb - 0.002, zb + 0.008, z)
    # Horizontal sag folds across the small of the back and the belly.
    folds = [(zb + 0.045 + rng.uniform(-0.01, 0.01), math.pi / 2 + rng.uniform(-0.4, 0.4), rng.uniform(0.6, 0.9), rng.uniform(-0.02, 0.02),
              0.0032, 0.008) for _ in range(2)]
    folds += [(zb + 0.1 + rng.uniform(-0.01, 0.02), math.pi / 2 + rng.uniform(-0.5, 0.5), rng.uniform(0.4, 0.7), rng.uniform(-0.03, 0.03),
               0.0025, 0.009)]
    folds += [(zb + 0.06 + rng.uniform(-0.01, 0.01), -math.pi / 2 + s * 0.6, 0.35, s * 0.02, 0.0022, 0.008) for s in (-1, 1)]
    d += ridges(z, psi, folds)
    # Under each arm: diagonal drape lines running down and in from the armpit, front and back.
    for side in (1.0, -1.0):
        base = 0.0 if side > 0 else math.pi
        for fb in (-1.0, 1.0):
            for i in range(2):
                a0 = base + side * fb * (0.32 + 0.16 * i)
                zz = k.z_a - 0.02 - 0.05 * i
                dp = wrap_pi(psi - a0)
                x = (z - zz) - side * fb * 0.09 * dp
                win = np.exp(-(dp / 0.28) ** 2) * np.exp(-((z - zz) / 0.07) ** 2)
                d += 0.0028 * win * (np.exp(-(x / 0.007) ** 2) - 0.3 * np.exp(-(x / 0.017) ** 2))
    # The back hangs from the shoulder blades: long soft vertical folds down to where it blouses over the band, and a
    # pair either side of the zip at the front, where the chest lets the cloth fall.
    # (Outward only near the band: the jeans' waist comes up under it when seated.)
    span = smoothstep(zb + 0.05, zb + 0.12, z) * (1.0 - smoothstep(k.z_a - 0.06, k.z_a + 0.02, z))
    for p0, lean, amp, w in ((0.5 * math.pi - 0.34, -0.25, 0.0034, 0.11), (0.5 * math.pi + 0.36, 0.22, 0.0031, 0.12),
                             (0.5 * math.pi + 0.02, 0.05, 0.0018, 0.09), (-0.5 * math.pi - 0.42, 0.3, 0.0022, 0.1),
                             (-0.5 * math.pi + 0.45, -0.28, 0.0022, 0.1)):
        x = wrap_pi(psi - p0 - lean * (z - k.z_a))
        trough = 0.35 * smoothstep(zb + 0.15, zb + 0.22, z)
        d += amp * span * (np.exp(-(x / w) ** 2) - trough * np.exp(-(x / (2.2 * w)) ** 2))
    return d


def ring_normals(P, centers):
    """Vertex normals of a (rows, cols, 3) ring grid (columns wrap), pointing away from each row's center (rows x 3)."""
    dc = np.roll(P, -1, axis=1) - np.roll(P, 1, axis=1)
    dr = np.empty_like(P)
    dr[1:-1] = P[2:] - P[:-2]
    dr[0] = P[1] - P[0]
    dr[-1] = P[-1] - P[-2]
    n = np.cross(dr, dc)
    n /= np.linalg.norm(n, axis=-1, keepdims=True) + 1e-12
    out = P - centers[:, None, :]
    s = np.sign((n * out).sum(-1, keepdims=True))
    s[s == 0] = 1.0
    return n * s


def arm_proxy(h, k, side):
    """Triangles standing in for an arm: the tee's sleeve and the body's bare arm (cut at the wrist)."""
    s = 'l' if side == 0 else 'r'
    sg = 1.0 if side == 0 else -1.0
    tee = k.tee_left if side == 0 else k.tee_right
    wrist, elbow = h.bone(f'hand_{s}'), h.bone(f'lowerarm_{s}')
    fd = (wrist - elbow) / np.linalg.norm(wrist - elbow)
    bt = h.bV[h.bF]
    ok = (sg * bt[..., 0].min(1) > 0.1) & (bt[..., 2].min(1) > 0.8) & (((bt - wrist) @ fd).max(1) < 0.012)
    V = np.concatenate([h.oV, h.bV])
    F = np.concatenate([h.oF[tee], h.bF[ok] + len(h.oV)])
    return V, F


def sleeve_stations(k, length, s_elbow):
    """Arc positions of the sleeve's rings: finer round the elbow and in the cuff's stack."""
    out = [0.0]
    while out[-1] < length - 1e-6:
        s = out[-1]
        ds = 0.006 if s > length - 0.16 else (0.0085 if abs(s - s_elbow) < 0.06 else 0.011)
        out.append(min(length, s + ds))
    if len(out) > 2 and out[-1] - out[-2] < 0.003:
        del out[-2]
    return np.array(out)


CUFF = 0.062   # the rib cuff's length


def build_sleeve(h, k, side, loopP, seam_i, dirs):
    """A sleeve from its armhole (loopP: the armhole's vertices in order, seam_i the one under the arm) to the cuff.
    Returns ring positions (stations x N), the axis, frames, arc positions and where the shell ends."""
    s = 'l' if side == 0 else 'r'
    shoulder, elbow, wrist = h.bone(f'upperarm_{s}'), h.bone(f'lowerarm_{s}'), h.bone(f'hand_{s}')
    fd = (wrist - elbow) / np.linalg.norm(wrist - elbow)
    P0 = loopP.mean(0)
    # Out of the armhole square to its plane, then bending down the arm.
    _, _, vt = np.linalg.svd(loopP - P0)
    n_ah = vt[2] * (1.0 if (vt[2][0] > 0) == (side == 0) else -1.0)
    ctrl = [P0, P0 + n_ah * 0.035, shoulder + 0.4 * (elbow - shoulder), elbow, wrist - 0.035 * fd, wrist - 0.013 * fd]
    dense = np.array(core.catmull_rom([tuple(p) for p in ctrl], 32))
    seg = np.linalg.norm(np.diff(dense, axis=0), axis=1)
    cum = np.concatenate([[0.0], np.cumsum(seg)])
    length = float(cum[-1])
    s_elbow = float(cum[np.argmin(np.linalg.norm(dense - elbow, axis=1))])
    st = sleeve_stations(k, length, s_elbow)
    C = np.stack([np.interp(st, cum, dense[:, i]) for i in range(3)], 1)
    T = np.gradient(C, axis=0)
    T /= np.linalg.norm(T, axis=1, keepdims=True)
    e1 = np.zeros_like(C)
    ref = np.array([0.0, 0.0, 1.0]) - T[0] * T[0][2]
    e1[0] = ref / np.linalg.norm(ref)
    for i in range(1, len(st)):
        v = e1[i - 1] - T[i] * (e1[i - 1] @ T[i])
        e1[i] = v / np.linalg.norm(v)
    e2 = np.cross(T, e1)
    # Sections of the arm (the tee's sleeve, the bare arm) across the axis: hull, centered on its own middle.
    V, F = arm_proxy(h, k, side)
    hulls, cen = [], []
    for i in range(len(st)):
        pts = section_points(V, F, C[i], T[i])
        pts = pts[np.linalg.norm(pts - C[i], axis=1) < 0.14]
        if len(pts) < 8:
            hulls.append(None)
            cen.append(None)
            continue
        q = np.stack([(pts - C[i]) @ e1[i], (pts - C[i]) @ e2[i]], 1)
        H = hull2d(q)
        hulls.append(H)
        cen.append(poly_centroid(H))
    hulls = fill_levels(hulls)
    cen = np.array(fill_levels(cen))
    cen = smooth1d(cen, 3, 0)
    Cc = C + e1 * cen[:, :1] + e2 * cen[:, 1:2]
    ease = np.interp(st, [0.0, s_elbow, length - CUFF - 0.12, length - CUFF], [0.027, 0.023, 0.022, 0.03])
    R = np.array([radial(H - c, (0.0, 0.0), THS) for H, c in zip(hulls, cen)])
    R = np.where(R > 0, R, np.nan)
    for i in range(len(R)):
        if np.isnan(R[i]).any():
            good = ~np.isnan(R[i])
            R[i] = np.interp(THS, THS[good], R[i][good], period=2 * math.pi) if good.any() else 0.05
    R = smooth1d(smooth1d(R, 2, 0), 4, 1, wrap=True) + ease[:, None]
    shell_end = length - CUFF
    inshell = st < shell_end
    R[inshell] = span_fill(R[inshell], 0.011, 0.5)
    R = smooth1d(R, 1.5, 0)
    # The rib cuff hugs the wrist; the shell gathers into it.
    iw = int(np.argmin(np.abs(st - (length - 0.02))))
    r_cuff = float(np.mean(R[iw] - ease[iw])) + 0.0075
    cuff = st >= shell_end
    R[cuff] = r_cuff
    gather = (st > shell_end - 0.014) & ~cuff
    w = smoothstep(shell_end - 0.014, shell_end, st[gather])[:, None]
    R[gather] = R[gather] * (1 - w) + (r_cuff + 0.004) * w
    # Ring 0 is the armhole; the angles ease from its own to evenly spaced, columns rising from the seam under the arm.
    N = len(loopP)
    rel = loopP - Cc[0]
    th0 = np.arctan2(rel @ e2[0], rel @ e1[0])
    if wrap_pi(th0[(seam_i + 1) % N] - th0[seam_i]) < 0:
        raise ValueError('armhole order runs against the sleeve frame')
    thu = th0[seam_i] + (np.arange(N) - seam_i) * 2 * math.pi / N
    th0 = thu + wrap_pi(th0 - thu)
    r0 = np.linalg.norm(rel - T[0] * (rel @ T[0])[:, None], axis=1)
    S_b = 0.08
    i_b = int(np.searchsorted(st, S_b))
    blend = smoothstep(S_b, 0.3, st)
    rings = [loopP]
    for i in range(1, len(st)):
        th = th0 + (thu - th0) * blend[i]
        tab_r = np.interp(np.mod(th, 2 * math.pi), THS, R[i], period=2 * math.pi)
        rings.append(Cc[i] + tab_r[:, None] * (np.cos(th)[:, None] * e1[i] + np.sin(th)[:, None] * e2[i]))
    # From the armhole to the first true sleeve ring, each column is a cubic: leaving the torso along its surface,
    # arriving along the sleeve.
    end = rings[i_b]
    gap = np.linalg.norm(end - loopP, axis=1, keepdims=True)
    chord = (end - loopP) / np.maximum(gap, 1e-9)
    lead = dirs + chord
    lead /= np.linalg.norm(lead, axis=1, keepdims=True)
    m0 = lead * gap * 1.1
    m1 = T[i_b][None] * gap * 1.1
    for i in range(1, i_b):
        u = st[i] / st[i_b]
        h00, h10, h01, h11 = 2 * u ** 3 - 3 * u ** 2 + 1, u ** 3 - 2 * u ** 2 + u, -2 * u ** 3 + 3 * u ** 2, u ** 3 - u ** 2
        rings[i] = h00 * loopP + h10 * m0 + h01 * end + h11 * m1
    P = np.array(rings)
    # Relax the rings leaving the armhole (the turn down the arm crowds them under it).
    nr = int(np.searchsorted(st, 0.1))
    for _ in range(4):
        Q = P.copy()
        avg = (np.roll(P, 1, 1) + np.roll(P, -1, 1))[1:nr] * 0.25 + (P[:nr - 1] + P[2:nr + 1]) * 0.25
        w = (1.0 - smoothstep(0.04, 0.1, st[1:nr]))[:, None, None] * 0.6
        Q[1:nr] = P[1:nr] * (1 - w) + avg * w
        P = Q
    return {'P': P, 'C': Cc, 'T': T, 'e1': e1, 'e2': e2, 'st': st, 'shell_end': shell_end, 'thu': thu, 's_elbow': s_elbow,
            'length': length, 'r_cuff': r_cuff, 'elbow': elbow, 'fd': fd, 'ud': (elbow - shoulder) / np.linalg.norm(elbow - shoulder)}


def sleeve_folds(sv, side):
    """The sleeve's folds: the stack just above the cuff, gathers into it, creases inside the elbow."""
    st, thu = sv['st'], sv['thu']
    S = np.broadcast_to(st[:, None], (len(st), len(thu)))
    TH = np.broadcast_to(thu[None, :], S.shape)
    rng = np.random.default_rng(31 + 5 * side)
    se = sv['shell_end']
    folds = []
    hh = 0.022 + rng.uniform(0.0, 0.006)
    while hh < 0.14:
        kk = 1.0 - smoothstep(0.02, 0.14, hh)
        for _ in range(int(rng.integers(1, 3))):
            folds.append((hh + rng.uniform(-0.005, 0.005), rng.uniform(-math.pi, math.pi), rng.uniform(0.8, 1.7),
                          rng.uniform(-0.012, 0.012), (0.0017 + 0.0033 * kk) * rng.uniform(0.8, 1.2), rng.uniform(0.005, 0.0075)))
        hh += rng.uniform(0.024, 0.036)
    d = ridges(se - S, TH, folds) * (S < se)
    d += 0.0022 * np.sin(13.0 * TH + 0.7 * np.sin(3 * TH)) * np.exp(-((S - se + 0.006) / 0.009) ** 2) * (S < se)
    # Inside the elbow: the frame angle of the bend's hollow side.
    i_e = int(np.argmin(np.abs(st - sv['s_elbow'])))
    bend = sv['fd'] - sv['ud']
    bend -= sv['T'][i_e] * (bend @ sv['T'][i_e])
    th_in = math.atan2(bend @ sv['e2'][i_e], bend @ sv['e1'][i_e])
    elbow_f = [(sv['s_elbow'] + o + rng.uniform(-0.004, 0.004), th_in + rng.uniform(-0.25, 0.25), rng.uniform(0.7, 1.0),
                rng.uniform(-0.01, 0.01), 0.0034 * rng.uniform(0.85, 1.1), 0.0065) for o in (-0.028, 0.0, 0.026)]
    d += ridges(S, TH, elbow_f)
    # A long soft drape down the underside of the upper arm.
    d += 0.0025 * np.exp(-(wrap_pi(TH - (thu[0] + 0.25)) / 0.35) ** 2) * np.exp(-((S - 0.12) / 0.06) ** 2)
    # The sleeve's twist: long diagonal folds spiralling round the upper arm and the forearm (the arm turns inside the
    # tube), broken and uneven, strongest toward the inside of the arm.
    twist = []
    for lo, hi, count in ((0.1, sv['s_elbow'] - 0.04, 4), (sv['s_elbow'] + 0.05, se - 0.11, 3)):
        if hi - lo < 0.04:
            continue
        for s0 in np.linspace(lo, hi, count) + rng.uniform(-0.012, 0.012, count):
            twist.append((s0, th_in + math.pi * rng.uniform(-0.55, 0.55), rng.uniform(0.9, 1.4),
                          rng.choice([-1.0, 1.0]) * rng.uniform(0.022, 0.034), 0.0036 * rng.uniform(0.75, 1.15), rng.uniform(0.0065, 0.009)))
    d += ridges(S, TH, twist) * (S < se - 0.02)
    d[0] = 0.0
    d *= smoothstep(0.0, 0.05, S)
    return d


def build_jacket(h, jf, jeans):
    k = jacket_fit(h, jf, jeans)
    tor = torso_table(h, k, jeans)
    M = JK_M
    dps = 2 * math.pi / M
    psi = -0.5 * math.pi + np.arange(M) * dps
    Z, ja, jS, zn = torso_rows(k, psi)
    k1, k2 = armhole_columns(h, k, tor)
    holes = [(k1, k2), (M - k2, M - k1)]
    PSI = column_angles(k, Z, k1, k2)
    P = tor.point(Z, PSI)
    # Over the shoulders to the neck seam, where the collar is sewn on.
    top = shoulder_rows(h, k, P[-1])
    P = np.concatenate([P, top])
    Z = P[..., 2]
    PSI = np.concatenate([PSI, np.arctan2(top[..., 1] - tor.center(top[..., 2])[..., 1], top[..., 0])])
    rng = np.random.default_rng(17)
    n = grid_normals(P, lambda z: tor.center(z))
    d = torso_folds(k, P, PSI, rng)
    # The zip's placket: a little fuller either side of center front.
    arc = wrap_pi(PSI + 0.5 * math.pi) * tor.radius(P[..., 2], PSI)
    d += 0.0016 * np.exp(-((np.abs(arc) - 0.011) / 0.006) ** 2) * (Z >= k.z_b)
    P = P + n * d[..., None]
    k.tor, k.psi, k.PSI, k.Z, k.ja, k.jS, k.zn, k.holes, k.P = tor, psi, PSI, Z, ja, jS, zn, holes, P
    B = Builder()
    G = B.verts(P, d)
    nb = len(BAND) - 1
    kmidL = (k1 + k2) // 2
    kmidR = M - kmidL
    k.kmid = (kmidL, kmidR)

    def tor_isl(r, c):
        if r < nb:
            return K_BAND_L if c < kmidL else K_BAND_B if c < kmidR else K_BAND_R
        return K_FRONT_L if c < kmidL else K_BACK if c < kmidR else K_FRONT_R

    for r in range(len(Z) - 1):
        for c in range(M):
            if ja <= r < jS and any(a <= c < b for a, b in holes):
                continue
            c1 = (c + 1) % M
            B.face((G[r, c], G[r, c1], G[r + 1, c1], G[r + 1, c]), RIB if r < nb else SHELL, tor_isl(r, c))
    # The sleeves, from their armholes.
    k.sleeves = []
    for side, (a, b) in enumerate(holes):
        # The armhole, round from under the arm: each vertex with the way the torso's surface runs into the hole there.
        pos = B.positions()
        cells = [(ja, c, [(ja - 1, c)] + ([(ja, a - 1)] if c == a else []) + ([(ja, b + 1)] if c == b else [])) for c in range(a, b + 1)]
        cells += [(j, b, [(j, b + 1)] + ([(jS + 1, b)] if j == jS else [])) for j in range(ja + 1, jS + 1)]
        cells += [(jS, c, [(jS + 1, c)] + ([(jS, a - 1)] if c == a else [])) for c in range(b - 1, a - 1, -1)]
        cells += [(j, a, [(j, a - 1)]) for j in range(jS - 1, ja, -1)]
        loop = [G[j, c] for j, c, _ in cells]
        dirs = []
        for j, c, ins in cells:
            v = sum(pos[G[j, c]] - pos[G[jj, cc % M]] for jj, cc in ins)
            dirs.append(v / np.linalg.norm(v))
        dirs = np.array(dirs)
        seam = (a + b) // 2 - a
        if side == 1:
            loop, dirs = loop[::-1], dirs[::-1]
            seam = len(loop) - 1 - seam
        loopP = pos[loop]
        try:
            sv = build_sleeve(h, k, side, loopP, seam, dirs)
        except ValueError:
            loop, dirs = loop[::-1], dirs[::-1]
            seam = len(loop) - 1 - seam
            loopP = pos[loop]
            sv = build_sleeve(h, k, side, loopP, seam, dirs)
        Ps = sv['P']
        ns = ring_normals(Ps, sv['C'])
        ds = sleeve_folds(sv, side)
        Ps = Ps + ns * ds[..., None]
        sv['P'], sv['n'], sv['seam'] = Ps, ns, seam
        Gs = np.concatenate([np.array(loop)[None], B.verts(Ps[1:], ds[1:])])
        sv['G'] = Gs
        N = len(loop)
        half = (np.arange(N) - seam) % N < N // 2
        cuff_rows = sv['st'] >= sv['shell_end'] - 1e-6
        fl, bl = (K_SLEEVE_LF, K_SLEEVE_LB) if side == 0 else (K_SLEEVE_RF, K_SLEEVE_RB)
        cf, cb = (K_CUFF_LF, K_CUFF_LB) if side == 0 else (K_CUFF_RF, K_CUFF_RB)
        for r in range(len(sv['st']) - 1):
            rib = cuff_rows[r] and cuff_rows[r + 1]
            for c in range(N):
                c1 = (c + 1) % N
                isl = (cf if half[c] else cb) if rib else (fl if half[c] else bl)
                B.face((Gs[r, c], Gs[r, c1], Gs[r + 1, c1], Gs[r + 1, c]), RIB if rib else SHELL, isl)
        # The cuff's end rolls under and runs back inside.
        end, ne = Ps[-1], ns[-1]
        Tn = sv['T'][-1]
        rA = B.verts(end + Tn * 0.0015 - ne * 0.0018)
        rB = B.verts(end - ne * 0.0042)
        inner = B.verts(Ps[cuff_rows][-2::-1] - ns[cuff_rows][-2::-1] * 0.0045)
        ci = K_CUFFIN_L if side == 0 else K_CUFFIN_R
        ring_strip(B, [Gs[-1], rA, rB], RIB, lambda c, cf=cf, cb=cb: cf if half[c] else cb)
        ring_strip(B, [rB] + list(inner), RIB, ci)
        sv["loop"] = list(loop)
        k.sleeves.append(sv)
    # The hem band's bottom edge rolls under, and its inner layer runs back up to the seam.
    nt = grid_normals(P, lambda z: tor.center(z))
    up = np.array([0.0, 0.0, 1.0])
    rA = B.verts(P[0] - up * 0.0015 - nt[0] * 0.0018)
    rB = B.verts(P[0] - nt[0] * 0.0045)
    inner = B.verts(P[1:nb + 1] - nt[1:nb + 1] * 0.0048)
    band_pieces = lambda c: K_BAND_L if c < kmidL else K_BAND_B if c < kmidR else K_BAND_R  # noqa: E731
    ring_strip(B, [G[0], rA, rB], RIB, band_pieces)
    ring_strip(B, [rB] + list(inner), RIB, lambda c: K_BANDIN_L if c < kmidL else K_BANDIN_B if c < kmidR else K_BANDIN_R)
    # The collar: rib, standing round the neck from the neckline, rolled over at the top, its inner layer back down.
    k.collar = build_collar(B, k, G[-1], P[-1], nt[-1])
    # The zip up center front, its slider and pull at the top.
    tree = BVHTree.FromPolygons([tuple(p) for p in B.positions()], B.faces)
    zip_path = np.concatenate([P[:, 0], k.collar['P'][1:, 0]])
    zip_n = np.concatenate([nt[:, 0], k.collar['n'][1:, 0]])
    zipper(B, k, zip_path, zip_n)
    k.welts = [welt_pocket(B, tree, k, side) for side in (0, 1)]
    obj = B.build(f'SK_Garment_Jacket_{h.key}')
    relax_around(obj, [v for sv in k.sleeves for v in sv['loop']], rings=4, iterations=10)
    orient_parts(obj)
    obj.data.shade_smooth()
    return obj, k


def ring_strip(B, rings, mat, isl):
    """Quads between consecutive closed rings of vertex indices; isl is an id or a function of the column."""
    for a, b in zip(rings, rings[1:]):
        N = len(a)
        for c in range(N):
            c1 = (c + 1) % N
            B.face((a[c], a[c1], b[c1], b[c]), mat, isl(c) if callable(isl) else isl)


def build_collar(B, k, base_idx, base_P, base_n):
    """The rib stand collar: from the neck seam (base_P, round the base of the neck) straight up round the neck, 5 cm
    (4 at the front), leaning forward with it. Its top hugs in toward the neck at the sides and back (no closer than
    an ellipse a finger's width off the neck) and stands clear of the throat at the front."""
    M = len(base_idx)
    c0 = np.array([0.0, k.neck_c[1]])
    rel = base_P[:, :2] - c0
    ang = np.arctan2(rel[:, 1], rel[:, 0])
    r_base = np.linalg.norm(rel, axis=1)
    a, b = k.neck_w + 0.012, k.neck_d + 0.012
    ell = a * b / np.sqrt((b * np.cos(ang)) ** 2 + (a * np.sin(ang)) ** 2)
    front = np.clip(-np.sin(ang), 0, 1) ** 2
    r_top = np.maximum(np.minimum(r_base - 0.003, ell), r_base - 0.02)
    r_top = r_top * (1 - front) + np.maximum(r_top, r_base + 0.004) * front
    r_top = smooth1d(r_top, 3, wrap=True)
    height = 0.05 - 0.01 * np.clip(-np.sin(ang), 0, 1)
    rows = [base_P]
    nc = 8
    for i in range(1, nc + 1):
        u = i / nc
        w = 1.0 - (1.0 - u) ** 1.8
        r = r_base * (1 - w) + r_top * w
        z = base_P[:, 2] + height * u
        y_lean = -0.010 * u
        rows.append(np.stack([c0[0] + r * np.cos(ang), c0[1] + y_lean + r * np.sin(ang), z], 1))
    Pc = np.array(rows)
    centers = np.stack([np.zeros(nc + 1), np.full(nc + 1, c0[1]), Pc[:, :, 2].mean(1)], 1)
    nc_n = ring_normals(Pc, centers)
    Gc = np.concatenate([np.array(base_idx)[None], B.verts(Pc[1:])])
    left = lambda c: K_COLLAR_L if c < M // 2 else K_COLLAR_R  # noqa: E731
    ring_strip(B, list(Gc), RIB, left)
    top, nt = Pc[-1], nc_n[-1]
    up = np.array([0.0, 0.0, 1.0])
    rA = B.verts(top + up * 0.0016 - nt * 0.002)
    rB = B.verts(top - nt * 0.0048)
    inner = B.verts(Pc[-2:0:-1] - nc_n[-2:0:-1] * 0.005)
    ring_strip(B, [Gc[-1], rA, rB], RIB, left)
    ring_strip(B, [rB] + list(inner), RIB, lambda c: K_COLLARIN_L if c < M // 2 else K_COLLARIN_R)
    return {'P': Pc, 'n': nc_n, 'G': Gc}


def zipper(B, k, path, normals):
    """A brass zip up center front: tape and teeth standing 2 mm proud, the slider and its pull at the top."""
    path = np.asarray(path)
    nn = np.asarray(normals)
    pos = path + nn * 0.0014
    seg = np.linalg.norm(np.diff(pos, axis=0), axis=1)
    keep = np.concatenate([[True], seg > 0.002])
    pos, nn = pos[keep], nn[keep]
    side = np.tile(np.array([1.0, 0.0, 0.0]), (len(pos), 1))
    strap(B, pos, side, 0.0024, 0.0135, K_ZIP, mat=HARDWARE)
    # Slider at the top, pull hanging from it.
    top, n_top = pos[-2], nn[-2]
    t_dir = pos[-1] - pos[-3]
    t_dir /= np.linalg.norm(t_dir)
    across = np.cross(t_dir, n_top)
    across /= np.linalg.norm(across)
    sl = [top + n_top * 0.002 - t_dir * 0.012 * u for u in np.linspace(0.0, 1.0, 5)]
    strap(B, sl, np.tile(across, (5, 1)), 0.0042, 0.0115, K_PULL, mat=HARDWARE)
    pull = [top + n_top * (0.0045 + 0.0025 * u) - t_dir * (0.012 + 0.03 * u) for u in np.linspace(0.0, 1.0, 7)]
    strap(B, pull, np.tile(across, (7, 1)), 0.0016, 0.0085, K_PULL, mat=HARDWARE)


def welt_pocket(B, tree, k, side):
    """A slanted hand pocket's welt: a strip of the shell, stitched down, standing 2.5 mm proud of the front."""
    sg = 1.0 if side == 0 else -1.0
    psi_c = -0.5 * math.pi + sg * 0.78
    zc = k.z_b + 0.105
    L, W = 0.15, 0.024
    ang = math.radians(68.0)
    tor = k.tor
    R = float(tor.radius(zc, psi_c))
    na, nbw = 24, 5
    A_, Bw = np.meshgrid(np.linspace(-L / 2, L / 2, na), np.linspace(-W / 2, W / 2, nbw))
    # Along the welt: up and toward the side seam.
    horiz = A_ * math.cos(ang) + Bw * math.sin(ang)
    vert = A_ * math.sin(ang) - Bw * math.cos(ang)
    psi = psi_c + sg * horiz / R
    z = zc + vert
    rough = tor.point(z, psi)
    P = np.zeros_like(rough)
    n = np.zeros_like(rough)
    for idx in np.ndindex(A_.shape):
        loc, nor, _, _ = tree.find_nearest(Vector(rough[idx]))
        nor = np.array(nor)
        cdir = np.array([loc.x, loc.y - tor.center(loc.z)[1], 0.0])
        if nor @ cdir < 0:
            nor = -nor
        P[idx], n[idx] = np.array(loc), nor
    n = smooth_grid(n)
    isl = K_WELT_L if side == 0 else K_WELT_R
    G = B.verts(P + n * 0.0026)
    B.grid(G, isl=isl, mat=SHELL)
    loop = list(G[0, :]) + list(G[1:, -1]) + list(G[-1, -2::-1]) + list(G[-2:0:-1, 0])
    loopN = np.concatenate([n[0, :], n[1:, -1], n[-1, -2::-1], n[-2:0:-1, 0]])
    loopP = B.positions()[loop]
    inner = B.verts(loopP - loopN * 0.0032)
    for i in range(len(loop)):
        i1 = (i + 1) % len(loop)
        B.face((loop[i1], loop[i], inner[i], inner[i1]), SHELL, isl)
    return {'psi': psi_c, 'z': zc, 'L': L, 'W': W, 'ang': ang, 'R': R, 'side': side}



def relax_around(obj, seeds, rings=4, iterations=10):
    """Smooths the surface a few edges either side of a seam (the armholes), strongest on the seam itself."""
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.verts.ensure_lookup_table()
    dist = {int(v): 0 for v in seeds}
    front = list(dist)
    for r in range(1, rings + 1):
        nxt = []
        for vi in front:
            for e in bm.verts[vi].link_edges:
                o = e.other_vert(bm.verts[vi]).index
                if o not in dist:
                    dist[o] = r
                    nxt.append(o)
        front = nxt
    for it in range(iterations):
        for r in range(rings, -1, -1):
            sel = [bm.verts[i] for i, d in dist.items() if d <= r]
            bmesh.ops.smooth_vert(bm, verts=sel, factor=0.35, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    bm.to_mesh(me)
    bm.free()
    me.update()



# ------------------------------------------------------------------ the jacket's textures

CANVAS = 0.6          # the shell's light neutral grey (linear): the game multiplies in the outfit's color
RIB_GREY = 0.5
TAPE = 0x2a2b2f
BRASS = 0x9c7a45
JACKET_SIZES = {SHELL: TEXTURE_SIZE, RIB: 1024, HARDWARE: 512}


def edge_distance(piece_img, valid, max_px=36):
    """Per texel, the distance in texels to the edge of its own pattern piece (nearest-point propagation)."""
    H, W = piece_img.shape
    pid = np.where(valid, piece_img, -1)
    seed = np.zeros((H, W), bool)
    for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        seed |= (pid >= 0) & (np.roll(pid, (dy, dx), (0, 1)) != pid)
    Y, X = np.mgrid[0:H, 0:W]
    by = np.where(seed, Y, -100000).astype(np.int32)
    bx = np.where(seed, X, -100000).astype(np.int32)
    d2 = np.where(seed, 0, 2 ** 30).astype(np.int64)
    shifts = [(1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)]
    for _ in range(max_px):
        changed = False
        for dy, dx in shifts:
            same = np.roll(pid, (dy, dx), (0, 1)) == pid
            cy = np.roll(by, (dy, dx), (0, 1))
            cx = np.roll(bx, (dy, dx), (0, 1))
            nd = (Y - cy).astype(np.int64) ** 2 + (X - cx).astype(np.int64) ** 2
            better = same & (pid >= 0) & (nd < d2) & (cy > -100000)
            if better.any():
                changed = True
                by = np.where(better, cy, by)
                bx = np.where(better, cx, bx)
                d2 = np.where(better, nd, d2)
        if not changed:
            break
    d = np.sqrt(d2.astype(np.float64))
    gy = np.where(d2 < 2 ** 30, Y - by, 0).astype(np.float64)
    gx = np.where(d2 < 2 ** 30, X - bx, 0).astype(np.float64)
    return d, gx, gy


def piece_frames(obj, slot, size, scale, iy, ix, piece):
    """Each texel's place in its pattern piece, in meters (+b up the garment), from the UV layout."""
    offs = {n: (p[2], p[3]) for n, p in enumerate(obj['uv_pieces'])}
    off = np.array([offs.get(int(p), (0.0, 0.0)) for p in range(int(piece.max()) + 1)]) if len(piece) else np.zeros((1, 2))
    u = (ix + 0.5) / size
    v = (iy + 0.5) / size
    a = (u - off[piece, 0]) / scale
    b = (v - off[piece, 1]) / scale
    return a, b


def jacket_textures(obj, k, name, uv_scale):
    """Paints the shell (canvas), the rib and the hardware, each into its own maps."""
    co = get_co(obj)
    nrm = vertex_normals(obj)
    fold = np.empty(len(obj.data.vertices), dtype=np.float32)
    obj.data.attributes['fold'].data.foreach_get('value', fold)
    set_attr(obj, 'gP', co + 2.0)
    set_attr(obj, 'gN', nrm * 0.5 + 0.5)
    set_attr(obj, 'gF', np.stack([fold * 100.0 + 0.5, np.zeros_like(fold), np.zeros_like(fold)], 1))
    _, _, mi, isl, _, _, _ = face_arrays(obj)
    piece = np.empty(len(obj.data.polygons), dtype=np.int64)
    obj.data.attributes['piece'].data.foreach_get('value', piece)
    set_attr(obj, 'gI', corner_values(obj, np.stack([isl / 32.0, piece / 256.0, np.zeros(len(isl))], 1)), 'CORNER')
    out = {}
    for slot in (SHELL, RIB, HARDWARE):
        t0 = time.time()
        size = JACKET_SIZES[slot]
        maps = bake_attrs(obj, size, ['gP', 'gN', 'gF', 'gI'], slot=slot)
        valid = maps['valid']
        iy, ix = np.nonzero(valid)
        P = maps['gP'][iy, ix].astype(np.float64) - 2.0
        N = maps['gN'][iy, ix].astype(np.float64) * 2.0 - 1.0
        N /= np.linalg.norm(N, axis=1, keepdims=True) + 1e-9
        fd = (maps['gF'][iy, ix, 0] - 0.5) / 100.0
        I = np.rint(maps['gI'][iy, ix, 0] * 32.0).astype(int)
        pc = np.rint(maps['gI'][iy, ix, 1] * 256.0).astype(int)
        a, b = piece_frames(obj, slot, size, uv_scale[slot], iy, ix, pc)
        px_m = uv_scale[slot] * size                      # texels per meter
        pimg = np.full((size, size), -1, dtype=np.int64)
        pimg[iy, ix] = pc
        d_px, gx, gy = edge_distance(pimg, valid, max_px=int(0.02 * px_m) + 2)
        de = d_px[iy, ix] / px_m                          # meters to the piece's edge
        tx, ty = -gy[iy, ix], gx[iy, ix]
        tn = np.hypot(tx, ty) + 1e-9
        along = (a * tx + b * ty) / tn                    # along the nearest edge
        fn = {SHELL: paint_shell, RIB: paint_rib, HARDWARE: paint_hardware}[slot]
        res = fn(k, P, N, fd, I, a, b, de, along)
        res.update({'iy': iy, 'ix': ix, 'P': P, 'valid': valid})
        out[slot] = res
        print(f'[garments] {name}: slot {slot} painted ({len(iy)} texels, {time.time() - t0:.0f}s)')
    return out


def paint_shell(k, P, N, fd, I, a, b, de, along):
    n = len(P)
    rng_n = texmaps.value_noise
    x, y, z = P[:, 0], P[:, 1], P[:, 2]
    welt = np.isin(I, [K_WELT_L, K_WELT_R])
    mott = rng_n(P, 0.05, seed=41, octaves=3)
    fine = rng_n(P, 0.003, seed=42, octaves=2)
    slub = rng_n(np.stack([a / 0.04, b / 0.0012, z * 0.0], 1) * 0.01, 0.01, seed=43, octaves=2)
    tone = np.full(n, CANVAS) * (1.0 + 0.07 * (mott - 0.5) + 0.05 * (slub - 0.5) + 0.04 * (fine - 0.5))
    height = np.zeros(n)
    rough = np.full(n, 0.8) + 0.05 * (fine - 0.5)
    # Seams: a groove at every piece's edge, the allowance pressed under it raising a band, a topstitch beside it.
    off = np.where(welt, 0.0022, 0.0055)
    height -= 0.00045 * np.exp(-(de / 0.0009) ** 2)
    height += 0.0003 * smoothstep(0.0012, 0.0025, de) * (1.0 - smoothstep(off + 0.002, off + 0.004, de))
    cov, hh = stitch_row(de - off, along, width=0.0009, period=0.0032)
    height += hh
    thread = cov
    tone = tone * (1.0 + 0.10 * np.exp(-((de - 0.002) / 0.0015) ** 2))       # seam ridges rub pale
    # Folds: ridges paler, hollows darker; fine creases in the hollows of the elbows and the gathers.
    tone *= 1.0 + 4.0 * np.clip(fd, -0.004, 0.004)
    crease = rng_n(np.stack([a / 0.02, b / 0.004, np.zeros(n)], 1) * 0.01, 0.01, seed=44, octaves=3)
    height += 0.00025 * (np.abs(crease - 0.5) * 2.0) * smoothstep(0.0005, 0.003, np.abs(fd))
    # Worn-in creases everywhere (canvas keeps them): thin ridged lines a few centimeters long, mostly across the
    # piece, crowding where the cloth folds; and a soft broad rumple under them.
    ridged = 1.0 - np.abs(rng_n(np.stack([a / 0.035, b / 0.009, np.zeros(n)], 1) * 0.01, 0.01, seed=45, octaves=3) - 0.5) * 2.0
    # A second set on the bias, so they don't all run one way, and patches where the cloth lies smooth.
    ua, ub = (a + b) * 0.7071, (b - a) * 0.7071
    ridged2 = 1.0 - np.abs(rng_n(np.stack([ua / 0.03, ub / 0.01, np.zeros(n)], 1) * 0.01, 0.01, seed=47, octaves=2) - 0.5) * 2.0
    rumple = rng_n(P, 0.045, seed=46, octaves=2)
    lie = smoothstep(0.3, 0.7, rng_n(P, 0.09, seed=48, octaves=2))
    creases = np.maximum(smoothstep(0.62, 0.95, ridged), 0.7 * smoothstep(0.7, 0.97, ridged2))
    height += (0.00012 + 0.00016 * lie + 0.00038 * smoothstep(0.0005, 0.003, np.abs(fd))) * creases + 0.0007 * (rumple - 0.5)
    tone *= 1.0 + 0.03 * (creases - 0.3) * (0.4 + 0.6 * lie)              # crease ridges catch a little wear
    # Grime low on the front and round the cuffs and pocket mouths; wear on the welts' edges.
    grime = 0.12 * (1.0 - smoothstep(k.z_b, k.z_b + 0.12, z)) * np.clip(-N[:, 1], 0, 1)
    grime += 0.1 * (mott > 0.62) * (mott - 0.62) * 4.0
    tone *= (1.0 - grime)
    tone = np.where(welt, tone * (1.0 + 0.12 * np.exp(-(de / 0.0025) ** 2)), tone)
    base = np.repeat(tone[:, None], 3, 1) * np.array([1.0, 0.985, 0.95])     # unbleached: a hair warm
    base = mix3(base, np.array([0.08, 0.075, 0.07]), thread * 0.85)          # dark thread (it isn't tinted away)
    rough = rough * (1 - thread) + 0.7 * thread
    return {'base': base, 'rough': rough, 'metal': np.zeros(n), 'height': height}


def paint_rib(k, P, N, fd, I, a, b, de, along):
    n = len(P)
    inside = np.isin(I, K_INSIDE)
    fine = texmaps.value_noise(P, 0.0015, seed=51, octaves=2)
    mott = texmaps.value_noise(P, 0.04, seed=52, octaves=2)
    # 2x2 rib: wales up the band 4.2 mm apart, the knit's courses across them.
    wale = 0.5 + 0.5 * np.cos(2 * math.pi * a / 0.0042)
    course = 0.5 + 0.5 * np.cos(2 * math.pi * b / 0.0016 + 2.0 * np.sin(2 * math.pi * a / 0.0021))
    height = 0.00065 * wale ** 0.7 + 0.00008 * course * wale + 0.00006 * (fine - 0.5)
    tone = RIB_GREY * (0.82 + 0.22 * wale) * (1.0 + 0.06 * (mott - 0.5) + 0.08 * (fine - 0.5))
    tone *= 1.0 - 0.18 * np.exp(-(de / 0.003) ** 2)         # its edges (the fold, the seam) in shadow
    tone = np.where(inside, tone * 0.8, tone)
    base = np.repeat(tone[:, None], 3, 1)
    rough = 0.88 - 0.05 * wale + 0.04 * (fine - 0.5)
    return {'base': base, 'rough': rough, 'metal': np.zeros(n), 'height': height}


def paint_hardware(k, P, N, fd, I, a, b, de, along):
    n = len(P)
    x, z = P[:, 0], P[:, 2]
    fine = texmaps.value_noise(P, 0.0008, seed=61, octaves=2)
    base = np.tile(lin(TAPE), (n, 1)) * (0.85 + 0.3 * fine)[:, None]
    rough = np.full(n, 0.85)
    metal = np.zeros(n)
    height = 0.00005 * (fine - 0.5)
    zip_ = I == K_ZIP
    front = zip_ & (np.abs(N[:, 0]) < 0.5) & (np.abs(x) < 0.0033)
    # Interlocking teeth, alternating from either tape, 3 mm apart.
    left = (x > -0.0033) & (x < 0.0005)
    right = (x < 0.0033) & (x > -0.0005)
    tl = left & (np.mod(z / 0.003, 1.0) < 0.62)
    tr = right & (np.mod(z / 0.003 + 0.5, 1.0) < 0.62)
    tooth = front & (tl | tr)
    ph = np.where(tl, np.mod(z / 0.003, 1.0), np.mod(z / 0.003 + 0.5, 1.0)) / 0.62
    dome = np.sin(np.pi * np.clip(ph, 0, 1)) * np.clip(1.0 - (np.abs(x) / 0.0033) ** 4, 0, 1)
    height += 0.0007 * dome * tooth
    worn = texmaps.value_noise(P, 0.002, seed=62, octaves=2)
    brass = mix3(np.tile(lin(BRASS), (n, 1)), np.tile(lin(0x4a3a22), (n, 1)), np.clip(0.35 - 0.6 * dome + 0.5 * (worn - 0.5), 0, 1))
    base = np.where(tooth[:, None], brass, base)
    metal = np.where(tooth, 1.0, metal)
    rough = np.where(tooth, 0.32 + 0.2 * (1 - dome), rough)
    # The tape's stitching to the jacket.
    tape_st = zip_ & (np.abs(np.abs(x) - 0.0056) < 0.0006) & (np.abs(N[:, 0]) < 0.5) & (np.mod(z / 0.0028, 1.0) < 0.7)
    base = np.where(tape_st[:, None], np.tile(lin(0x55565a), (n, 1)), base)
    # Slider and pull: brass, rubbed bright on the edges.
    pull = I == K_PULL
    pb = mix3(np.tile(lin(BRASS), (n, 1)), np.tile(lin(0x3d3020), (n, 1)), np.clip(0.4 - 0.3 * np.exp(-(de / 0.0015) ** 2) + 0.4 * (worn - 0.5), 0, 1))
    base = np.where(pull[:, None], pb, base)
    metal = np.where(pull, 1.0, metal)
    rough = np.where(pull, 0.3 + 0.15 * worn, rough)
    height += 0.0002 * pull * smoothstep(0.0005, 0.002, de)
    return {'base': base, 'rough': rough, 'metal': metal, 'height': height}


def maps_material(name, res, size, specular=0.3):
    return texture_material(name, res, size, specular=specular)


def finish_jacket(h, jacket, k, bake=True):
    name = jacket.name
    for nm in ('shell', 'rib', 'zip'):
        jacket.data.materials.append(core.Mat(f'{name}_{nm}_placeholder').m)
    unwrap(jacket)
    k.uv_scale = layout_uvs(jacket, JACKET_SIZES)
    res = jacket_textures(jacket, k, name, k.uv_scale)
    mats = [texture_material(f'M_{name}_{nm}', res[s], JACKET_SIZES[s], specular=sp)
            for s, nm, sp in ((SHELL, 'shell', 0.32), (RIB, 'rib', 0.3), (HARDWARE, 'zip', 0.5))]
    for s, m in enumerate(mats):
        jacket.material_slots[s].material = m
    if bake:
        bake_garment(jacket, name, {mats[RIB].name: JACKET_SIZES[RIB], mats[HARDWARE].name: JACKET_SIZES[HARDWARE]}, ao_distance=0.03)


def strip_bake_attrs(obj):
    for nm in ('gP', 'gN', 'gF', 'gI'):
        if nm in obj.data.attributes:
            obj.data.attributes.remove(obj.data.attributes[nm])



def skin_jacket(h, jacket, k, jeans):
    """The jacket's weights from the tee (torso, collar, hem) and the bare arms (to the wrist; never the hands)."""
    keep = allowed_bones(h.bone_names, JACKET_BONES)

    def arm_tris(V, F):
        T = V[F]
        ok = np.zeros(len(F), bool)
        for s, sg in (('l', 1.0), ('r', -1.0)):
            wrist, elbow = h.bone(f'hand_{s}'), h.bone(f'lowerarm_{s}')
            fd = (wrist - elbow) / np.linalg.norm(wrist - elbow)
            ok |= (sg * T[..., 0].min(1) > 0.1) & (T[..., 2].min(1) > 0.8) & (((T - wrist) @ fd).max(1) < 0.01)
        return ok

    # The hem band rides on the jeans (their weights take it over the hips and thighs as they move).
    sources = [(h.outfit, lambda V, F: h.o_shirt), (h.body, arm_tris), (jeans, lambda V, F: V[F][:, :, 2].min(1) > k.z_hem - 0.05)]
    W = transfer_weights(h, jacket, sources, keep, smooth_iter=6)
    bind(h, jacket, W)
    return W


def detail_sheet(h, objs, path, f, k):
    """Close looks at the details under the review rig."""
    pose_reset(h)
    views = [('collar', -20, 12, 0.75, (0.0, -0.08, 1.48)), ('shoulder', 60, 15, 0.9, (0.2, 0.0, 1.32)),
             ('cuff', -40, 10, 0.6, tuple(h.bone('hand_l') + [0.0, 0.02, 0.04])), ('welt', -25, 5, 0.65, (0.12, -0.15, k.z_b + 0.1)),
             ('seat', 165, 8, 0.9, (0.0, 0.1, f.z_split + 0.1)), ('hem', 25, 15, 0.8, (0.13, -0.04, 0.1))] if k else \
            [('seat', 165, 8, 0.9, (0.0, 0.1, f.z_split + 0.1)), ('hem', 25, 15, 0.8, (0.13, -0.04, 0.1))]
    p = look(h, objs, os.path.join(core.BUILD_DIR, 'review', 'garments', os.path.basename(path).replace('.jpg', '.png')), views, tile=600, samples=64)
    px = review_pixels(p)
    save_rgb(path, px)
    print(f'[garments] detail sheet {os.path.relpath(path, core.ROOT)}')
    return path



# The creator's outfit colors (ss::hero::OutfitTone), for showing the jacket as the game tints it.
OUTFIT_TONES = {'charcoal': 0x34383f, 'navy': 0x1f2d4d, 'olive': 0x4b5638, 'burgundy': 0x6a1f2e, 'sand': 0xbfa883,
                'black': 0x15161a, 'teal': 0x1f6f6a, 'mustard': 0xc79a2e}
TINT_LIFT, RIB_SHADE = 1.5, 0.55   # as AShortStackCharacter tints them (TintSlot's lift; the rib a darker shade)


def review_tint(jacket, tone):
    """Multiplies the jacket's baked shell (and, darker, its rib) by an outfit color, as the game's material does."""
    rgb = np.array(core.hex_linear(tone)[:3]) * TINT_LIFT
    for slot, k in ((SHELL, 1.0), (RIB, RIB_SHADE)):
        m = jacket.material_slots[slot].material
        nt = m.node_tree
        bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
        sock = bsdf.inputs['Base Color']
        if not sock.is_linked:
            continue
        src = sock.links[0].from_socket
        mix = nt.nodes.new('ShaderNodeMix')
        mix.data_type = 'RGBA'
        mix.blend_type = 'MULTIPLY'
        mix.inputs[0].default_value = 1.0
        nt.links.new(src, mix.inputs[6])
        mix.inputs[7].default_value = (*(rgb * k), 1.0)
        nt.links.new(mix.outputs[2], sock)



# ------------------------------------------------------------------ the weave, for Unreal to tile

DETAIL_TILE = 0.01   # meters a detail map covers


def periodic_noise(u, v, n_terms=24, fmax=24, seed=0):
    """Tileable noise on the unit square: a sum of sines with whole-number frequencies."""
    rng = np.random.default_rng(seed)
    out = np.zeros_like(u)
    for _ in range(n_terms):
        fx, fy = rng.integers(-fmax, fmax + 1, 2)
        a = 1.0 / (1.0 + math.hypot(fx, fy))
        out += a * np.sin(2 * math.pi * (fx * u + fy * v) + rng.uniform(0, 2 * math.pi))
    return out / (np.abs(out).max() + 1e-9)


def write_details():
    """Tiling weave normals for M_Garment's Detail (1 cm square, 512 px): a 3/1 right-hand twill for the denim (its wales
    climbing to the right at ~63 degrees, the warp's ends showing), a plain-weave duck for the jacket's canvas."""
    n = 512
    u, v = np.meshgrid((np.arange(n) + 0.5) / n, (np.arange(n) + 0.5) / n)
    P = np.stack([u * DETAIL_TILE, v * DETAIL_TILE, np.zeros_like(u)], -1)
    valid = np.ones((n, n), bool)
    fib = periodic_noise(u, v, 40, 60, 3)
    twill = (0.5 + 0.5 * np.cos(2 * math.pi * (6 * u - 3 * v))) ** 1.6
    ends = 0.5 + 0.5 * np.cos(2 * math.pi * 30 * u + 0.8 * periodic_noise(u, v, 12, 8, 4))
    h_twill = 0.00011 * twill + 0.00003 * ends * twill + 0.000015 * fib
    duck_w = 0.5 + 0.5 * np.cos(2 * math.pi * 16 * u)
    duck_f = 0.5 + 0.5 * np.cos(2 * math.pi * 16 * v)
    over = np.cos(2 * math.pi * 8 * u) * np.cos(2 * math.pi * 8 * v)
    h_duck = 0.00007 * (duck_w * (0.5 + 0.5 * over) + duck_f * (0.5 - 0.5 * over)) + 0.000012 * fib
    dst = os.path.join(GARMENT_DIR, 'Textures')
    os.makedirs(dst, exist_ok=True)
    import shutil
    out = []
    for name, hgt in (('T_Garment_Detail_Twill_N', h_twill), ('T_Garment_Detail_Canvas_N', h_duck)):
        nm = texmaps.height_to_normal(hgt, P, valid)
        p = texmaps.save(name, nm * 0.5 + 0.5, 'Non-Color')
        shutil.copyfile(p, os.path.join(dst, f'{name}.png'))
        out.append(name)
    return out


def write_notes(name, uv_scale):
    """garments.json beside the FBX files: each garment's UV scale per slot and the weave's tiling that matches it."""
    import json
    path = os.path.join(GARMENT_DIR, 'garments.json')
    notes = {}
    if os.path.isfile(path):
        with open(path, 'r', encoding='utf-8') as fh:
            notes = json.load(fh)
    notes[name] = {'uv_per_meter': {str(s): round(float(v), 5) for s, v in uv_scale.items()},
                   'detail_tiling': {str(s): round(1.0 / (float(v) * DETAIL_TILE), 3) for s, v in uv_scale.items()}}
    os.makedirs(GARMENT_DIR, exist_ok=True)
    with open(path, 'w', encoding='utf-8', newline='\n') as fh:
        json.dump(notes, fh, indent=2, sort_keys=True)
        fh.write('\n')
    return path



def compose_overview():
    """art/review/garments.jpg: both bodies' sheets side by side (A left, B right), once both exist."""
    paths = [os.path.join(core.REVIEW_DIR, f'garments_{k}.jpg') for k in HEROES]
    if not all(os.path.isfile(p) for p in paths):
        return None
    imgs = [review_pixels(p) for p in paths]
    hh = min(im.shape[0] for im in imgs)
    gap = np.full((hh, 12, 4), 0.05, dtype=np.float32)
    out = np.concatenate([imgs[0][:hh], gap, imgs[1][:hh]], axis=1)
    path = os.path.join(core.REVIEW_DIR, 'garments.jpg')
    save_rgb(path, out)
    return path


if __name__ == '__main__':
    main()
