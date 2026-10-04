"""What the Embercrest's people wear on their heads and faces: caps, a beanie, a trilby, glasses and two kinds of shades.

The game fits each piece to a MetaHuman at runtime (ABackRoomPlayer::ApplyWear): the origin is the midpoint
between the eyes and the piece is scaled by the face's eye spacing (6.3 cm here), so one mesh fits every head.
The colors are the person's: the "wear_main" and "wear_frame" materials are recolored per person; "wear_trim",
"wear_lens" and "wear_metal" keep theirs.

Coordinates (meters): origin between the eyes, +X out of the face, +Z up; the head is roughly an ellipsoid centered
7.5 cm behind the eyes and 2 cm above them, 20 x 15 x 20 cm, so hats sit a little outside that.
"""
import math

import bmesh
import bpy
from mathutils import Matrix, Vector

from artkit import core, parts

MESHES = ['SM_Wear_Cap', 'SM_Wear_Beanie', 'SM_Wear_Trilby', 'SM_Wear_Glasses', 'SM_Wear_Shades', 'SM_Wear_Aviators']
DOUBLE_SIDED = True
# Recolored per person in the game: plain materials, exported unbaked.
NO_BAKE = True
REVIEW_VIEWS = [('front', -150, 12, 0.5, (0.0, 0.0, 0.02)), ('side', -80, 10, 0.5, (-0.06, 0.0, 0.02))]
REVIEW_SCREEN_LIGHT = False

HEAD_C = Vector((-0.075, 0.0, 0.02))


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


def build():
    core.reset()
    M = mats()
    return [cap(M), beanie(M), trilby(M), glasses(M), shades(M), aviators(M)]
