"""What the Embercrest's people wear on their heads and faces: caps, a beanie, a trilby, glasses and two kinds of shades;
and the sponsors' patches they sew on (next_segment_spec 3.10).

The game fits each piece to a MetaHuman at runtime (ABackRoomPlayer::ApplyWear): the origin is the midpoint
between the eyes and the piece is scaled by the face's eye spacing (6.3 cm here), so one mesh fits every head.
The colors are the person's: the "wear_main" and "wear_frame" materials are recolored per person; "wear_trim",
"wear_lens" and "wear_metal" keep theirs.

Coordinates (meters): origin between the eyes, +X out of the face, +Z up; the head is roughly an ellipsoid centered
7.5 cm behind the eyes and 2 cm above them, 20 x 15 x 20 cm, so hats sit a little outside that.

The patches (SM_Wear_Patch 7.6 x 5.1 cm for the chest, SM_Wear_PatchRound 6.4 cm for the sleeve, SM_Wear_PatchCap 5.6 x
3.8 cm) are 2.5 mm thick at their merrowed edge and bent to what they're sewn on (CHEST_BEND, SLEEVE_BEND, CAP_BEND): the
chest's gently (0.30 m across, 0.45 m up: its sides 2.4 mm and its corners 3 mm behind its middle), the sleeve's round
the upper arm (7.5 cm across, straight along it: its sides 7 mm back), the cap's to the crown. Origin at the middle of the
back (the side that lies on the garment, where it touches), +X out of the patch, +Y to the wearer's left as you face them
(the art's right), +Z up: on the sleeve +Z runs along the arm toward the shoulder, the axis it is bent round. Slot 0 is the
face (its UV0 spans the face's art, 0..1, the cap's and the chest's the same art), slot 1 the merrowed edge (UV0: u
centimetres along the edge, v 0..1 across it, inside to back). M_Patch (segment_art_setup.py) gives every slot its look
from the embroidered atlas sponsors.py writes (unreal/Art/Patches), by the brand's index. A few hundred triangles each.
"""
import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

from artkit import core, parts, review

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import sponsors  # noqa: E402

PATCHES = ['SM_Wear_Patch', 'SM_Wear_PatchRound', 'SM_Wear_PatchCap']
MESHES = ['SM_Wear_Cap', 'SM_Wear_Beanie', 'SM_Wear_Trilby', 'SM_Wear_Glasses', 'SM_Wear_Shades', 'SM_Wear_Aviators'] + PATCHES
DOUBLE_SIDED = True
# Recolored per person in the game, or (the patches) given M_Patch: plain materials, exported unbaked.
NO_BAKE = True
# The head wear stands in a row facing the camera for review (pose_for_review); the patches get a sheet of their own.
ROW = {'SM_Wear_Cap': -0.6, 'SM_Wear_Beanie': -0.36, 'SM_Wear_Trilby': -0.12, 'SM_Wear_Glasses': 0.12, 'SM_Wear_Shades': 0.32, 'SM_Wear_Aviators': 0.52}
REVIEW_VIEWS = [('front', -12, 10, 2.5, (-0.05, 0.0, 0.06)), ('top', 18, 48, 2.6, (-0.05, 0.0, 0.04)), ('detail', -40, 16, 0.55, (-0.6, 0.0, 0.07))]
REVIEW_SCREEN_LIGHT = False

HEAD_C = Vector((-0.075, 0.0, 0.02))

# The patches: the twill's thickness, how far the face's middle puffs up, and the merrowed edge's section as
# (in from the patch's edge, up off its back), metres, from the foot on the face round the roll to the back.
FACE_T = 0.0014
PILLOW = 0.00025
BEAD = [(0.0018, FACE_T - 0.00005), (0.0009, 0.0024), (-0.0003, 0.0025), (-0.0008, 0.0011), (-0.0004, 0.0)]
# The merrow's wraps per centimetre (M_Patch's Wraps): the thread overcast round the edge at a slant.
WRAPS_PER_CM = 14.0
# Where the back of a patch reads its colour: the face's twill just inside the satin edge.
BACK_UV = (0.5, 0.045)
# Each patch is bent to what it's sewn on, as a sewn patch follows the cloth: radius across and up (metres; None is flat).
# Measured on the garments (unreal/Art/Garments, bind pose): the chest at the wearer's left runs 0.5 m across and 0.35 m
# up on body A's hoodie and bomber, 0.1 to 0.16 m on body B's and the extras'; the upper sleeve's outside, 60 to 70% of
# the way from the shoulder joint to the elbow, runs 6 to 8.5 cm round the arm and is straight along it to within 2 mm
# (higher, over the deltoid and the sleeve's fold, it bulges: no place for a patch). There a flat 6.4 cm disc's back
# stood 8 to 13 mm further off the sleeve at one place than another; bent round 7.5 cm, 2.5 to 6 mm. The cap's is its crown.
CHEST_BEND = (0.30, 0.45)
SLEEVE_BEND = (0.075, None)
CAP_BEND = (0.065, 0.09)


def plain(name, hex_color, rough, metal=0.0):
    m = core.Mat(name)
    m.set('Base Color', core.hex_linear(hex_color))
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    return m


def mats():
    return {
        'main': plain('wear_main', 0x7a7a7a, 0.88),
        'trim': plain('wear_trim', 0x1c1c1e, 0.7),
        'frame': plain('wear_frame', 0x141414, 0.25),
        'lens': plain('wear_lens', 0x040506, 0.3),
        'metal': plain('wear_metal', 0xc9a24a, 0.22, 1.0),
    }


def mesh_from_bm(name, bm, mat):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    core.link(obj)
    core.assign(obj, mat)
    return obj


def solidify(obj, thickness, offset=-1.0):
    mod = obj.modifiers.new('solid', 'SOLIDIFY')
    mod.thickness = thickness
    mod.offset = offset
    mod.use_even_offset = True
    core.select_only([obj], obj)
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj


def dome(name, center, axes, front_z, slope, mat, thickness, segs=72, rings=48, crease=0.0, ribs=0.0):
    """An ellipsoid cap cut by a plane tilted front to back (z = front_z + slope * (x - 0.03)), kept above it."""
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segs, v_segments=rings, radius=1.0)
    for v in bm.verts:
        p = Vector((center[0] + v.co.x * axes[0], center[1] + v.co.y * axes[1], center[2] + v.co.z * axes[2]))
        if crease > 0.0 and v.co.z > 0.0:
            # A trilby's pinch: the top pressed in along the middle, front to back.
            p.z -= crease * math.exp(-(v.co.y / 0.28) ** 2) * v.co.z
        if ribs > 0.0:
            ang = math.atan2(v.co.y, v.co.x)
            p += Vector((math.cos(ang), math.sin(ang), 0.0)) * ribs * math.cos(ang * 64)
        v.co = p
    normal = Vector((-slope, 0.0, 1.0)).normalized()
    point = Vector((0.03, 0.0, front_z))
    bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=point, plane_no=normal, clear_inner=True)
    obj = mesh_from_bm(name, bm, mat)
    return solidify(obj, thickness)


def cut_z(x, front_z, slope):
    return front_z + slope * (x - 0.03)


def cap(M):
    front_z, slope = 0.056, 0.21
    # Headroom over the crown of the head (heads run a couple of centimeters taller than the 20 cm template).
    crown = dome('crown', (HEAD_C.x, 0.0, 0.034), (0.112, 0.090, 0.104), front_z, slope, M['main'], 0.003)
    objs = [crown]
    top = Vector((HEAD_C.x + 0.004, 0.0, 0.034 + 0.104))
    button = parts.disc('button', top + Vector((0, 0, 0.002)), (0, 0, 1), 0.007, 0.004, 24)
    core.assign(button, M['trim'])
    objs.append(button)
    # The brim: a half disc off the front, curved down at the sides, tipped a little down.
    outer = [(0.075 * math.cos(t), 0.098 * math.sin(t)) for t in [math.pi * (-0.5 + i / 40) for i in range(41)]]
    outline = outer + [(-0.03, 0.08), (-0.03, -0.08)]
    brim = parts.outline_prism('brim', outline, z0=-0.002, z1=0.002)
    for v in brim.data.vertices:
        x, y, z = v.co
        v.co = Vector((x, y, z - 1.8 * y * y - 0.10 * max(x, 0.0)))
    brim.data.transform(Matrix.Translation((0.015, 0.0, cut_z(0.015, front_z, slope) + 0.004)))
    core.assign(brim, M['main'])
    objs.append(brim)
    return parts.finish(objs, 'SM_Wear_Cap')


def beanie(M):
    front_z, slope = 0.052, 0.30
    cx, cz, ax, ay, az = HEAD_C.x + 0.002, 0.030, 0.111, 0.090, 0.112
    body = dome('knit', (cx, 0.0, cz), (ax, ay, az), front_z, slope, M['main'], 0.004)
    # The folded cuff: a band round the bottom edge that hugs the knit, just proud of it, following the head in.
    bm = bmesh.new()
    n, rows, proud = 96, 6, 0.0035

    def on_knit(a, z):
        s = math.sqrt(max(0.0, 1.0 - ((z - cz) / az) ** 2))
        return cx + math.cos(a) * (ax * s + proud), math.sin(a) * (ay * s + proud)

    grid = []
    for k in range(n):
        a = 2 * math.pi * k / n
        z0 = cut_z(cx + math.cos(a) * ax, front_z, slope)
        col = []
        for r in range(rows + 1):
            z = z0 - 0.001 + 0.040 * r / rows
            x, y = on_knit(a, z)
            col.append(bm.verts.new((x, y, z)))
        grid.append(col)
    for k in range(n):
        for r in range(rows):
            bm.faces.new((grid[k][r], grid[(k + 1) % n][r], grid[(k + 1) % n][r + 1], grid[k][r + 1]))
    cuff = solidify(mesh_from_bm('cuff', bm, M['main']), 0.005, 1.0)
    return parts.finish([body, cuff], 'SM_Wear_Beanie')


def trilby(M):
    front_z, slope = 0.058, 0.20
    # Tall enough that the pinch along the top still clears a tall head.
    crown = dome('crown', (HEAD_C.x, 0.0, 0.060), (0.106, 0.086, 0.100), front_z, slope, M["main"], 0.003, crease=0.014)
    # The band, then the brim: up at the back, snapped down at the front.
    bm = bmesh.new()
    n = 96
    lo, hi = [], []
    for k in range(n):
        a = 2 * math.pi * k / n
        x = HEAD_C.x + math.cos(a) * 0.1075
        y = math.sin(a) * 0.0875
        z0 = cut_z(x, front_z, slope)
        lo.append(bm.verts.new((x, y, z0 + 0.003)))
        hi.append(bm.verts.new((x, y, z0 + 0.028)))
    for k in range(n):
        bm.faces.new((lo[k], lo[(k + 1) % n], hi[(k + 1) % n], hi[k]))
    band = solidify(mesh_from_bm('band', bm, M['trim']), 0.0015, 1.0)
    bm = bmesh.new()
    inner, outer = [], []
    for k in range(n):
        a = 2 * math.pi * k / n
        ix, iy = HEAD_C.x + math.cos(a) * 0.104, math.sin(a) * 0.084
        ox, oy = HEAD_C.x + math.cos(a) * 0.152, math.sin(a) * 0.128
        zi = cut_z(ix, front_z, slope) + 0.003
        front = max(math.cos(a), 0.0)
        back = max(-math.cos(a), 0.0)
        zo = cut_z(ox, front_z, slope) + 0.003 - 0.016 * front ** 2 + 0.018 * back ** 2 + 0.004 * abs(math.sin(a))
        inner.append(bm.verts.new((ix, iy, zi)))
        outer.append(bm.verts.new((ox, oy, zo)))
    for k in range(n):
        bm.faces.new((inner[k], inner[(k + 1) % n], outer[(k + 1) % n], outer[k]))
    brim = solidify(mesh_from_bm('brim', bm, M['main']), 0.003, 0.0)
    return parts.finish([crown, band, brim], 'SM_Wear_Trilby')


# Glasses: drawn in the face's plane (y across, z up) and pushed out along x.
FACE = Matrix(((0, 0, 1, 0), (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 0, 1)))


def face_prism(name, outline, holes, x0, x1, mat):
    obj = parts.outline_prism(name, outline, holes, z0=x0, z1=x1, matrix=FACE)
    core.assign(obj, mat)
    return obj


def rect_outline(cy, cz, w, h, r):
    return core.rounded_rect(cy - w / 2, cz - h / 2, cy + w / 2, cz + h / 2, r, steps=8)


def trapezoid(cy, cz, w, h, r, top_extra, side):
    # Wider at the top and toward the temple: a wayfarer's lens.
    pts = rect_outline(cy, cz, w, h, r)
    out = []
    for y, z in pts:
        t = (z - cz) / h + 0.5
        out.append((y + side * (y - cy) * top_extra * t * 0.4 + (y - cy) * top_extra * (t - 0.5), z))
    return out


def teardrop(cy, cz, w, h, side, n=48):
    out = []
    for k in range(n):
        a = 2 * math.pi * k / n
        y = math.cos(a) * w / 2
        z = math.sin(a) * h / 2
        # Deeper toward the cheek and the temple.
        if z < 0:
            z *= 1.0 + 0.25 * max(0.0, side * y / (w / 2))
            y += side * 0.004 * (-z / (h / 2))
        out.append((cy + y, cz + z))
    return out


def temples(M, mat, spread=0.077, r=0.0021):
    objs = []
    for side in (1, -1):
        # Hinged at the frame's corner, then angled out round the temple to sit on the ear.
        pts = [(0.022, side * 0.066, 0.009), (0.008, side * 0.069, 0.009), (-0.03, side * (spread - 0.002), 0.008),
               (-0.07, side * spread, 0.005), (-0.092, side * (spread - 0.001), -0.004), (-0.102, side * (spread - 0.004), -0.02)]
        t = parts.tube('temple', pts, r, 8, 6)
        core.assign(t, mat)
        objs.append(t)
        hinge = parts.rbox('hinge', (0.021, side * 0.0645, 0.009), (0.006, 0.006, 0.008), r=0.001)
        core.assign(hinge, mat)
        objs.append(hinge)
    return objs


def glasses(M):
    objs = []
    for side in (1, -1):
        cy = side * 0.032
        hole = rect_outline(cy, -0.002, 0.050, 0.034, 0.011)
        rim = rect_outline(cy, -0.002, 0.050 + 0.0065, 0.034 + 0.006, 0.013)
        objs.append(face_prism('rim', rim, [hole], 0.0225, 0.0263, M['frame']))
    bridge = parts.tube('bridge', [(0.025, -0.008, 0.008), (0.027, 0.0, 0.011), (0.025, 0.008, 0.008)], 0.0024, 8, 6)
    core.assign(bridge, M['frame'])
    objs.append(bridge)
    objs += temples(M, M['frame'])
    return parts.finish(objs, 'SM_Wear_Glasses')


def shades(M):
    objs = []
    for side in (1, -1):
        cy = side * 0.033
        hole = trapezoid(cy, -0.002, 0.052, 0.038, 0.010, 0.10, side)
        rim = trapezoid(cy, -0.002, 0.052 + 0.008, 0.038 + 0.008, 0.013, 0.10, side)
        objs.append(face_prism('rim', rim, [hole], 0.0222, 0.0267, M['frame']))
        objs.append(face_prism('lens', hole, [], 0.0235, 0.0255, M['lens']))
    bridge = parts.rbox('bridge', (0.026, 0.0, 0.010), (0.006, 0.016, 0.008), r=0.002)
    core.assign(bridge, M['frame'])
    objs.append(bridge)
    objs += temples(M, M['frame'], r=0.0022)
    return parts.finish(objs, 'SM_Wear_Shades')


def aviators(M):
    objs = []
    for side in (1, -1):
        cy = side * 0.033
        lens = teardrop(cy, -0.004, 0.056, 0.046, side)
        rim = teardrop(cy, -0.004, 0.0585, 0.0485, side)
        objs.append(face_prism('rim', rim, [lens], 0.023, 0.025, M['metal']))
        objs.append(face_prism('lens', lens, [], 0.0235, 0.0245, M['lens']))
    for z, bow in ((0.017, 0.002), (0.008, -0.0015)):
        b = parts.tube('bridge', [(0.025, -0.008, z), (0.026, 0.0, z + bow), (0.025, 0.008, z)], 0.0009, 6, 6)
        core.assign(b, M['metal'])
        objs.append(b)
    objs += temples(M, M['metal'], r=0.0012)
    return parts.finish(objs, 'SM_Wear_Aviators')


# ------------------------------------------------------------------ the sponsors' patches


def patch_outline(shape, dims, e, splits=(0, 0, 0, 0), steps=6, n=32):
    """The patch's outline moved in by e (metres; negative is out), counter-clockwise as you face it, in (across, up).
    Every e gives the same points in the same order (a rounded rectangle keeps its corners' centres), so the rings of the
    merrowed edge line up. splits: extra points along the right, top, left and bottom sides (for bending)."""
    if shape == 'round':
        r = dims / 2 - e
        return [(r * math.cos(2 * math.pi * i / n - math.pi / 2), r * math.sin(2 * math.pi * i / n - math.pi / 2)) for i in range(n)]
    w, h = dims
    rc = sponsors.CORNER_MM / 1000.0
    cx, cy = w / 2 - rc, h / 2 - rc
    rr = rc - e
    corners = [((cx, -cy), -math.pi / 2), ((cx, cy), 0.0), ((-cx, cy), math.pi / 2), ((-cx, -cy), math.pi)]
    pts = []
    for i, ((ox, oy), a0) in enumerate(corners):
        arc = [(ox + rr * math.cos(a0 + 0.5 * math.pi * j / steps), oy + rr * math.sin(a0 + 0.5 * math.pi * j / steps)) for j in range(steps + 1)]
        pts += arc
        (nx, ny), na = corners[(i + 1) % 4]
        nxt = (nx + rr * math.cos(na), ny + rr * math.sin(na))
        for k in range(1, splits[i] + 1):
            t = k / (splits[i] + 1)
            pts.append((arc[-1][0] + (nxt[0] - arc[-1][0]) * t, arc[-1][1] + (nxt[1] - arc[-1][1]) * t))
    return pts


def patch(name, shape, dims, rings=(0.5,), splits=(0, 0, 0, 0), bend=None):
    """A patch: its face (the twill, puffed a little in the middle), the merrowed edge rolled over its rim, and its back.
    Built in (across, up, out) and turned so out is +X; bend=(across, up) radii wraps it round a crown."""
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    base = patch_outline(shape, dims, 0.0, splits)
    if shape == 'round':
        span = (dims, dims)
    else:
        span = dims
    N = len(base)

    def face_uv(a, b):
        return ((a + span[0] / 2) / span[0], (b + span[1] / 2) / span[1])

    def vert(a, b, n):
        return bm.verts.new((a, b, n))
    # The face: the outline, rings drawn in toward the middle, and the middle; higher toward the middle.
    loops = []
    for s in (1.0,) + tuple(rings):
        z = FACE_T + PILLOW * (1.0 - s * s)
        loops.append([vert(a * s, b * s, z) for a, b in base])
    middle = vert(0.0, 0.0, FACE_T + PILLOW)
    faces = []
    for outer, inner in zip(loops, loops[1:]):
        for i in range(N):
            j = (i + 1) % N
            faces.append((bm.faces.new((outer[i], outer[j], inner[j], inner[i])), 0))
    for i in range(N):
        faces.append((bm.faces.new((loops[-1][i], loops[-1][(i + 1) % N], middle)), 0))
    for f, _ in faces:
        f.material_index = 0
        f.smooth = True
        for lp in f.loops:
            lp[uv].uv = face_uv(lp.vert.co.x, lp.vert.co.y)
    # The merrowed edge: its section swept round the outline; u is centimetres along it, v goes from the foot to the back.
    lengths = [0.0]
    for i in range(N):
        a, b = base[i], base[(i + 1) % N]
        lengths.append(lengths[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
    rings_b = [[vert(a, b, z) for a, b in patch_outline(shape, dims, e, splits)] for e, z in BEAD]
    for k in range(len(BEAD) - 1):
        for i in range(N):
            j = (i + 1) % N
            f = bm.faces.new((rings_b[k][i], rings_b[k + 1][i], rings_b[k + 1][j], rings_b[k][j]))
            f.material_index = 1
            f.smooth = True
            u0, u1 = lengths[i] * 100.0, lengths[i + 1] * 100.0
            v0, v1 = k / (len(BEAD) - 1), (k + 1) / (len(BEAD) - 1)
            for lp, t in zip(f.loops, ((u0, v0), (u0, v1), (u1, v1), (u1, v0))):
                lp[uv].uv = t
    # The back, flat on the garment: one point of the face's twill.
    back = bm.faces.new(list(reversed(rings_b[-1])))
    back.material_index = 0
    back.smooth = False
    for lp in back.loops:
        lp[uv].uv = BACK_UV
    if bend:
        # Round a cylinder across (its axis along up, behind the patch), then one along up: the middle of the back stays
        # at the origin and the rim curls back toward the garment. Lengths along the back are kept, so the art isn't
        # stretched.
        rh, rv = bend
        for v in bm.verts:
            a, b, n = v.co
            if rh:
                th = a / rh
                a, n = (rh + n) * math.sin(th), (rh + n) * math.cos(th) - rh
            if rv:
                ph = b / rv
                b, n = (rv + n) * math.sin(ph), (rv + n) * math.cos(ph) - rv
            v.co = (a, b, n)
    # (across, up, out) -> (Y, Z, X): out of the patch is +X, a proper turn (no mirror).
    for v in bm.verts:
        a, b, n = v.co
        v.co = (n, a, b)
    obj = core.mesh_object(name, bm)
    # Plain stand-ins, named for their slots (patch_face, patch_thread): the editor gives them M_Patch.
    for slot, color, rough in (('patch_face', 0x3a3d44, 0.8), ('patch_thread', 0x8a8d94, 0.45)):
        m = bpy.data.materials.get(slot)
        if m is None:
            mat = core.Mat(slot)
            mat.set('Base Color', core.hex_linear(color))
            mat.set('Roughness', rough)
            m = mat.m
        obj.data.materials.append(m)
    return obj


def patches():
    w, h = sponsors.WIDE_MM
    cw, ch = sponsors.CAP_MM
    return [patch('SM_Wear_Patch', 'wide', (w / 1000.0, h / 1000.0), rings=(0.5,), splits=(1, 2, 1, 2), bend=CHEST_BEND),
            patch('SM_Wear_PatchRound', 'round', sponsors.ROUND_MM / 1000.0, rings=(0.66, 0.33), bend=SLEEVE_BEND),
            patch('SM_Wear_PatchCap', 'wide', (cw / 1000.0, ch / 1000.0), rings=(0.66, 0.33), splits=(2, 3, 2, 3), bend=CAP_BEND)]


def build():
    # The atlas first (its renders need a scene of their own), then the meshes.
    core.reset()
    sponsors.build_atlas()
    core.reset()
    M = mats()
    return [cap(M), beanie(M), trilby(M), glasses(M), shades(M), aviators(M)] + patches()


# ------------------------------------------------------------------ review


def _atlas(key):
    return os.path.join(sponsors.OUT, f'T_PatchAtlas_{key}.png')


def preview_face(b, shape):
    """A patch face as M_Patch draws it: brand b's cell of the atlas (Blender's UVs run up, the atlas's rows down)."""
    x, y, w, h = (sponsors.wide_cell if shape == 'wide' else sponsors.round_cell)(b.index)
    A = float(sponsors.ATLAS)
    m = core.Mat(f'preview_{b.id}_{shape}')
    tc = m.node('ShaderNodeTexCoord')
    mp = m.node('ShaderNodeMapping')
    mp.vector_type = 'POINT'
    mp.inputs['Scale'].default_value = (w / A, h / A, 1.0)
    mp.inputs['Location'].default_value = (x / A, (A - y - h) / A, 0.0)
    m.link(tc.outputs['UV'], mp.inputs['Vector'])
    col = m.image(_atlas('BaseColor'), vector=mp.outputs['Vector'], extension='EXTEND')
    nrm = m.image(_atlas('Normal'), non_color=True, vector=mp.outputs['Vector'], extension='EXTEND')
    ort = m.image(_atlas('ORT'), non_color=True, vector=mp.outputs['Vector'], extension='EXTEND')
    sep = m.node('ShaderNodeSeparateColor')
    m.link(ort.outputs['Color'], sep.inputs['Color'])
    nm = m.node('ShaderNodeNormalMap')
    m.link(nrm.outputs['Color'], nm.inputs['Color'])
    m.set('Base Color', col.outputs['Color'])
    m.set('Roughness', sep.outputs['Green'])
    m.set('Normal', nm.outputs['Normal'])
    m.set('Specular IOR Level', m.math('ADD', 0.3, m.math('MULTIPLY', sep.outputs['Blue'], 0.25)))
    return m


def preview_thread(b):
    """The merrowed edge as M_Patch draws it: the brand's thread overcast at a slant."""
    m = core.Mat(f'preview_thread_{b.id}')
    tc = m.node('ShaderNodeTexCoord')
    sep = m.node('ShaderNodeSeparateXYZ')
    m.link(tc.outputs['UV'], sep.inputs['Vector'])
    f = m.math('ADD', m.math('MULTIPLY', sep.outputs['X'], WRAPS_PER_CM), m.math('MULTIPLY', sep.outputs['Y'], 0.6))
    h = m.math('ABSOLUTE', m.math('SINE', m.math('MULTIPLY', f, math.pi)))
    base = core.hex_linear(b.thread)
    m.set('Base Color', m.mix(h, (base[0] * 0.55, base[1] * 0.55, base[2] * 0.55, 1.0), base))
    m.set('Roughness', 0.42)
    bump = m.node('ShaderNodeBump', Strength=0.6, Distance=0.0003)
    m.link(h, bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    return m


def _shown(src, name, b, shape, matrix):
    """A copy of patch src wearing brand b, placed by matrix (for review only)."""
    dup = src.copy()
    dup.data = src.data.copy()
    dup.name = name
    core.link(dup)
    dup.data.materials[0] = preview_face(b, shape).m
    dup.data.materials[1] = preview_thread(b).m
    dup.matrix_world = matrix
    dup.hide_render = False
    return dup


def sleeve_swatch(name, x, z, radius, half_angle, height, y_front, mat):
    """A strip of sleeve for review: a cylinder's front (axis along Z, radius metres) spanning +-half_angle, its middle at
    (x, y_front, z) facing -Y."""
    bm = bmesh.new()
    n = 24
    cols = []
    for i in range(n + 1):
        a = -half_angle + 2.0 * half_angle * i / n
        px, py = x + radius * math.sin(a), y_front + radius * (1.0 - math.cos(a))
        cols.append([bm.verts.new((px, py, z - height / 2)), bm.verts.new((px, py, z + height / 2))])
    for i in range(n):
        f = bm.faces.new((cols[i][0], cols[i + 1][0], cols[i + 1][1], cols[i][1]))
        f.smooth = True
    return mesh_from_bm(name, bm, mat)


def patch_sheet(src, cap_src):
    """art/review/wear_patches.jpg: every brand's chest patch on a denim board, its sleeve patch on a strip of sleeve (the
    round patch is bent round an arm), the cap patch on a cap."""
    face_cam = Matrix.Rotation(-math.pi / 2, 4, 'Z')  # +X (out of the patch) toward the review camera (-Y)
    m = core.Mat('review_denim')
    tc = m.node('ShaderNodeTexCoord')
    wave = m.node('ShaderNodeTexWave', Scale=900.0, Distortion=1.5)
    m.link(tc.outputs['Object'], wave.inputs['Vector'])
    m.set('Base Color', m.mix(wave.outputs['Fac'], core.hex_linear(0x1f2a3d), core.hex_linear(0x2f3d55)))
    m.set('Roughness', 0.9)
    shown = []
    # The bent chest patch stands off a flat board by as much as its rim curls back, so its rim touches the board.
    lift = -min(v.co.x for v in src['SM_Wear_Patch'].data.vertices)
    half = math.radians(27.0)
    sleeve_front = -SLEEVE_BEND[0] * (1.0 - math.cos(half))
    for b in sponsors.BRANDS:
        col, row = b.index % 4, b.index // 4
        at = Matrix.Translation((-0.40 + col * 0.088, -0.0002 - lift, 0.25 - row * 0.062))
        shown.append(_shown(src['SM_Wear_Patch'], f'show_{b.id}_wide', b, 'wide', at @ face_cam))
        x, z = 0.04 + col * 0.075, 0.25 - row * 0.075
        shown.append(sleeve_swatch(f'review_sleeve_{b.id}', x, z, SLEEVE_BEND[0], half, 0.07, sleeve_front, m))
        at = Matrix.Translation((x, sleeve_front - 0.0002, z))
        shown.append(_shown(src['SM_Wear_PatchRound'], f'show_{b.id}_round', b, 'round', at @ face_cam))
    board = parts.rbox('review_denim', (-0.07, 0.003, 0.13), (0.82, 0.006, 0.36), r=0.002)
    core.assign(board, m)
    shown.append(board)
    # Volt Rush on the cap's crown, where the crown's surface stands 34.5 degrees back from upright.
    cap_obj = cap_src.copy()
    cap_obj.data = cap_src.data.copy()
    cap_obj.name = 'show_cap'
    core.link(cap_obj)
    cap_obj.hide_render = False
    cap_obj.data.materials[0] = preview_cap_cloth().m
    cap_obj.matrix_world = Matrix.Translation((0.0, -0.2, -0.055)) @ Matrix.Rotation(math.radians(-115), 4, 'Z')
    on_cap = _shown(src['SM_Wear_PatchCap'], 'show_cap_patch', sponsors.BY_ID['voltrush'], 'wide', Matrix())
    on_cap.parent = cap_obj
    on_cap.matrix_parent_inverse = Matrix()
    on_cap.matrix_basis = Matrix.Translation((0.0198, 0.0, 0.09)) @ Matrix.Rotation(math.radians(-34.5), 4, 'Y')
    shown += [cap_obj, on_cap]
    bpy.context.view_layer.update()
    lo, hi = core.bounds(shown)
    size = (hi - lo).length

    def view(label, az, el, dist, target):
        return (label, az, el, dist / size, target)
    views = [view('front', -6, 6, 0.95, (-0.268, 0.0, 0.157)), view('round', 6, 6, 0.8, (0.1525, 0.0, 0.1375)),
             view('detail', -24, 14, 0.24, (-0.30, 0.0, 0.19))]
    path = review.sheet(shown, 'wear_patches', views=views, screen_light=False)
    print(f'[wear] review sheet {os.path.relpath(path, core.ROOT)}')
    for o in shown:
        bpy.data.objects.remove(o)


def preview_cap_cloth():
    m = core.Mat('review_cap_cloth')
    m.set('Base Color', core.hex_linear(0x20314f))
    m.set('Roughness', 0.88)
    return m


def pose_for_review(objs):
    """The patches' own sheet first (wear_patches.jpg); then the head wear in a row facing the camera, for wear.jpg."""
    by = {o.name: o for o in objs}
    src = {n: by[n] for n in PATCHES if n in by}
    heads = [o for o in objs if o.name not in PATCHES]
    for o in objs:
        o.hide_render = True
    if len(src) == len(PATCHES) and 'SM_Wear_Cap' in by:
        patch_sheet(src, by['SM_Wear_Cap'])
    for o in heads:
        o.hide_render = False
        o.matrix_world = Matrix.Translation((ROW.get(o.name, 0.0), 0.0, 0.0)) @ Matrix.Rotation(-math.pi / 2, 4, 'Z')
    objs[:] = heads
