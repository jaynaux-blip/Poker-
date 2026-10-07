"""SM_NS_Thrift_*: Second Chance Thrift, 220 Market St (plot M-S2), a former five-and-dime run by St. Agnes Outreach
(world realism spec section 4.6). The module also carries the shop kit (geometry helpers, the per-face finish system,
print atlases, the unbaked slots the game drives, the bake split and the review renderer) that ns_gym.py imports.

Frames (spec 0.4). SITE meshes (Blender origin = the stage origin; place at the stage's transform, yaw 180; stage
(X, Y, Z) cm = (-100x, 100y, 100z)): the front and the room. PROP meshes: front toward Blender -X, placed at stage L
with yaw F + 180 to face stage yaw F (in a SITE scene that is Blender yaw -F: S2B()). Meters, Z up. Prints read
correctly from outside their face (spec 0.3), checked on the review sheets.

The plot (spec 2.4): the opening X -2440..-3660 in the Y 4950 plane, Z 0..400; the room X -2460..-3640, Y 3650..4920,
the drop ceiling at 330; the double doors X -2960..-3140, recessed 1.2 m (their plane Y 4830).

Slots: baked slots are named <prefix>_<group> before the bake (Unreal: M_<Mesh>_<i>); the unbaked ones keep their names:
  ns_glass        [glass] (M_NS_Glass): the front's panes, the doors' and the case's glass. Nanite off for those meshes.
  ns_mirror       [mirror] (M_NS_Mirror): the three-way mirror's panels, UV 0..1 across each as seen; the material's
                  base color is the desilvering mask (T_NSTH_MirrorMask).
  ns_screen_tag   [screen]: the rack tag's two faces, UV 0..1 across each as read from its side (core draws TEES, ...).
  glow_*          the _Glow<Family> meshes (one white emissive slot, the game colors it).
The build log prints every mesh's slot order.

SITE meshes:
- SM_NS_Thrift_Front (baked: body, print): in the opening. Two display windows (bronze-anodized frames, a center
  mullion each, black structural-glass bulkheads on a bronze shoe, one panel replaced with painted board) either side
  of angled glass returns into a 1.2 m recess; the double doors' frame and transom (the house number 220 on a plaque);
  the recess floor in 1 in mosaic with the old store's name, HALLORAN'S 5 & 10, reading from the sidewalk; a beadboard
  soffit with a can light; a prism-glass transom band (two rows of tiles in bronze cames, Z 300..328); the painted sign
  band Z 332..400 (SECOND CHANCE THRIFT / St. Agnes Outreach, CLOTHING · SHOES and HATS · GLASSES at the ends: what the
  racks hold) in a molded frame, three gooseneck lamps over it; inside the glass the posters YELLOW TAGS 50% OFF
  SATURDAYS (west) and DONATIONS IN BACK (east), paper backs toward the room; carpeted mannequin platforms (top Z 50)
  behind paneled knee walls (to Z 108), a track of three spots over each window; window dressing (a hard suitcase,
  books, a vase, a bentwood chair). An awning, if ue-world hangs one, mounts below Z 300 (the transom band).
- SM_NS_Thrift_Front_Glass ([glass]): the display panes and returns (Y 4940), the door transom (Y 4830).
- SM_NS_Thrift_Front_GlowWarm: the six window spots' lenses, the recess can, the three goosenecks' lenses. The lights to
  add (MegaLights): six 3000 K spots of about 40 deg aimed at the platforms from (X, Y, Z) at the spot heads (Y 4895,
  Z 319: X -2540, -2700, -2860 (west) and -3240, -3400, -3560 (east)), a 2800 K downlight at (-3050, 4895, 292), three
  2800 K floods on the sign from X -2700, -3050, -3400, Y 5002, Z 407.
- SM_NS_Thrift_Shell (baked: floor, wall, ceil, trim, print): worn VCT (12 in, a dark border course, replacement
  tiles, traffic paths worn pale from the door to the register, the racks and the booth, wax dark at the walls);
  walls sage below a wood chair rail, cream above, vinyl cove base; a 2 x 4 drop ceiling at Z 330 (each tile its own
  stain, one tile pushed up), ten troffers, two supply diffusers and a return, sprinkler heads, a smoke detector; the
  pegboard on the east wall with belts, purses and scarves; the staff shelving behind the counter (donations waiting);
  the steel donation door at the back (X -3100..-3192) with DONATIONS stenciled, a STAFF ONLY sign, a lit EXIT sign
  over it and another hung over the front doors; an extinguisher, a pull station, switches, a thermostat; the
  hand-lettered ALL SALES FINAL notice by the counter.
- SM_NS_Thrift_Shell_GlowCool: the ten troffer lenses (4000-4100 K), at stage (X, Y) (-2776 | -3324, 3744 | 3988 |
  4232 | 4476 | 4720), Z 325.

PROP meshes (origin on the floor under the middle unless noted):
- SM_NS_Thrift_Door / _Door_R: the leaves (origin on the hinge axis at the floor; _Door spans +Y from it, _R -Y; the
  street face -X). Closed, both at stage yaw F 90: _Door's hinge at (X -2962, Y 4830), _R's at (X -3138, Y 4830).
  They swing in: _Door by raising F toward 180, _R by lowering it toward 0. Brass PUSH plates outside, pulls inside,
  a St. Agnes decal on the west leaf.
- SM_NS_Thrift_RackRound (Ø 1.2 m chrome ring at 1.37 m on a telescoping post and a caster cross): hanger points
  H0..H61 (round_rack_points()): (0.6 cos a, 0.6 sin a, 1.386), a = i * 360/62 deg from +X, garment yaw a - 90.
- SM_NS_Thrift_RackStraight (two 1.8 m rails at X +-0.28, 1.37 m, H-frame ends on casters): H0..H28 on the -X rail
  from Y -0.84 to +0.84 every 6 cm, H29..H57 on the +X rail, Z 1.3835, garment yaw 90.
  A hung garment (its hook at the point) at that yaw hangs across the rail, as on a real rack.
- SM_NS_Thrift_Table (6 ft church folding table, top 0.74): folded-stack positions T0..T7 at (+-0.19, -0.66 | -0.22 |
  0.22 | 0.66, 0.74); a PLEASE REFOLD card taped on.
- SM_NS_Thrift_ShoeWall (an 8 ft slatwall on the wall plane x = 0, toward -X, 25 shelves): shoe points SH0..SH24
  (shoe_points()): x -0.11, rows Z 0.62, 0.96, 1.30, 1.64, 1.98, columns Y -0.96, -0.48, 0, 0.48, 0.96; a sneaker
  (sneakers.py's frame) goes at z + 0.085, yaw 0 (toe +Y), its outside toward the room.
- SM_NS_Thrift_HatWall (a 4 ft slatwall, 12 hat forms): hat points HT0..HT11 (hat_points()): (-0.27, Y -0.42 | -0.14 |
  0.14 | 0.42, Z 1.295 | 1.655 | 2.015), yaw -90 for a wearables.py hat (origin at the top of the head inside).
- SM_NS_Thrift_Case (1.22 x 0.51 x 0.97 showcase; customer -X): sunglasses, costume necklaces on a bust, brooches,
  bangles, watches on a pillow, ASK TO SEE / Bea has the key. The glasses sold come from the hat wall's use; the case
  is dressing.
- SM_NS_Thrift_Counter (1.83 x 0.61, top 0.97; customer -X): the register (its pole display to the customer), a tag
  gun and tags, paper bags (one open, stamped), a jar of buttons, PAY HERE, a call bell, the food-pantry can, a pen
  cup, a receipt spike; the staff side's shelves and the church-station radio. Bea stands at +X.
- SM_NS_Thrift_Booth (1.0 x 1.0 booth of painted panels opening toward -X; the three-way mirror on a plinth beside it
  at +Y): the stool's seat top at (0.22, -0.22, 0.46); the hero stands at MIRROR_AT (-1.05, 1.32) facing +X to see
  himself in all three panels (thrift-fitting orbits there). The rail runs across the front at Z 2.06.
- SM_NS_Thrift_Booth_Curtain: drawn shut, origin at the rings' top at the rail's -Y end; place it at the booth's
  CURTAIN_AT (-0.54, -0.50, 2.06) and scale Y from about 0.25 (open, gathered) to 1.0 (shut).
- SM_NS_Thrift_Tags: one hanging sign (origin at the ceiling where its cables clip on: place at Z 330), the sign 0.76 x
  0.25 hanging 0.45..0.70 below, both faces [screen]. One per rack and wall (TAGS_AT).
- SM_NS_Thrift_Mannequin (stance A) and _Mannequin_B (P1): 1.86 m jointless mannequins on round bases (sizes in
  mannequin()); for the windows' platforms at Z 50.
- SM_NS_Thrift_Misc (P1, a wall group: origin on the floor at the wall plane, toward -X, Y -2.25..2.25): the bookcase
  with books and LPs, a crate of records, the furniture corner (a mustard armchair, an unlit floor lamp, a console CRT
  marked AS IS), three framed prints, St. Agnes' bulletin board of flyers, the donation bin (thrift-donate) with
  DONATIONS on it.
- Garment stand-ins, until looks' statics land (spec 4.7 frames and slots, so the swap is a name):
  SM_NS_Thrift_Hung_Tee / _Hoodie / _Jacket / _Pants (origin at the hook's inside top, where it rests on a rail; front
  -X, shoulders along Y; slots 0 cloth [tint] baked light neutral grey, 1 hanger) and SM_NS_Thrift_Folded_Tee / _Jeans /
  _Hoodie (stacks of three, origin at the bottom middle; slot 0 cloth [tint]).
  looks' statics (garments.py: SM_NS_Rack_Tee / _Hoodie / _Bomber / _Jeans / _Chinos and SM_NS_Folded_Tee / _Jeans /
  _Hoodie) share these frames; ThriftStage hangs a SM_NS_Rack_* wherever one is exported and falls back to the stand-ins
  (the jacket stand-in covers the P1 flannel, denim, leather and track racks until they land). The store review dresses
  the racks the same way (LOOKS_RACK).

LAYOUT below is the review's floor plan (stage cm and yaw), a suggestion for ThriftStage: the counter and case on the
west side by the door, the hat wall on the west wall, the shoe wall on the back wall, round racks (tops) by the
windows, the tees table, the outerwear and pants racks mid-floor, the booth and mirror in the back-east corner ("on
the left"), the misc wall on the east wall.

Picks inside the spec's ranges, and departures: the doors are two meshes (_Door, _Door_R: a pair's handing can't be a
mirror); the curtain is its own mesh so the game can draw it; the stand-in garments and the mannequins' second stance
are extra meshes. Tri counts (the build log): the front 25.7k, the shell 50.0k, the round rack 5.7k, the straight rack
4.6k, the table 2.2k, the shoe wall 8.8k, the hat wall 6.0k, the case 14.8k, the booth 7.3k + its curtain 12.3k, the
counter 8.4k, the mannequins 9.4k each, the misc wall 20.7k, the tags 0.76k. Several run under the table's targets
where the shapes are done (the front, the counter, the case, the walls, the misc wall): detail went into the bakes, not
into flat faces.

THRIFT_ONLY=front,shell,... (keys of NAMES) builds just those; THRIFT_REVIEW=0 skips the store and close-look sheets.
"""
import math
import os
import random
import shutil

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts, review, texmaps
from artkit.shade import convex_edges, mix_float, noise, object_coords, ramp, scaled, smooth_less, vec
from artkit.sheet import Sheet

MODULE = 'ns_thrift'


# ================================================================== frames

def S(X, Y, Z=0.0):
    """Stage centimeters -> SITE Blender meters (spec 0.4: Blender x = -X/100, y = Y/100, z = Z/100)."""
    return Vector((-X / 100.0, Y / 100.0, Z / 100.0))


def H(h, a=1.0):
    return core.hex_linear(h, a)


def lin3(h):
    c = core.hex_linear(h)
    return (c[0], c[1], c[2])


# ================================================================== geometry helpers

def set_attr(obj, name, value):
    """A per-face attribute on every face of obj (a float, or a 3-vector)."""
    me = obj.data
    n = len(me.polygons)
    is_vec = isinstance(value, (tuple, list, Vector))
    a = me.attributes.get(name)
    if a is None:
        a = me.attributes.new(name, 'FLOAT_VECTOR' if is_vec else 'FLOAT', 'FACE')
    if is_vec:
        v = tuple(value)
        a.data.foreach_set('vector', [float(v[i % 3]) for i in range(n * 3)])
    else:
        a.data.foreach_set('value', [float(value)] * n)


def bx(name, x0, x1, y0, y1, z0, z1, r=0.0, seg=2):
    """An axis-aligned box between the given bounds, its edges rounded by r."""
    return parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), r, seg)


def bxc(name, c, size, r=0.0, seg=2, rot=None):
    return parts.rbox(name, c, size, r, seg, rot)


def weld(obj, dist=1e-6):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=dist)
    bm.to_mesh(obj.data)
    bm.free()


def lathe_solid(name, profile, matrix=None, segments=48):
    """A closed solid of revolution about Z from a closed (r, z) loop, moved by matrix."""
    obj = core.lathe(name, list(profile) + [profile[0]], segments)
    weld(obj)
    core.orient_normals(obj)
    if matrix is not None:
        obj.data.transform(matrix)
    return obj


def lathe_axis(name, profile, matrix=None, segments=48):
    """A solid of revolution about Z whose (r, z) profile starts and ends on the axis."""
    obj = core.lathe(name, profile, segments)
    weld(obj)
    core.orient_normals(obj)
    if matrix is not None:
        obj.data.transform(matrix)
    return obj


def tube(name, pts, r, n=12, closed=False):
    return core.sweep(name, pts, parts.circle(r, n), closed=closed, caps=not closed)


def rod(name, a, b, r, n=16):
    return parts.rod(name, a, b, r, n)


def bend(points, radius, steps=6):
    """A 3D polyline with its corners rounded (bent tube, wire)."""
    P = [Vector(p) for p in points]
    out = [P[0]]
    for i in range(1, len(P) - 1):
        a, c, b = P[i - 1], P[i], P[i + 1]
        d0, d1 = a - c, b - c
        l0, l1 = d0.length, d1.length
        d0.normalize()
        d1.normalize()
        ang = math.acos(max(-1.0, min(1.0, d0.dot(d1))))
        if ang > math.pi - 1e-3 or ang < 1e-3:
            out.append(c)
            continue
        t = min(radius / math.tan(ang / 2), l0 * 0.49, l1 * 0.49)
        p0, p1 = c + d0 * t, c + d1 * t
        for k in range(steps + 1):
            s = k / steps
            out.append((1 - s) ** 2 * p0 + 2 * (1 - s) * s * c + s ** 2 * p1)
    out.append(P[-1])
    return [tuple(p) for p in out]


def frame_matrix(u, v, origin):
    """Local X -> u, local Y -> v, local Z -> u x v, at origin."""
    U, V = Vector(u).normalized(), Vector(v).normalized()
    N = U.cross(V).normalized()
    m = Matrix((U, V, N)).transposed().to_4x4()
    m.translation = Vector(origin)
    return m


def M(x=0.0, y=0.0, z=0.0, yaw=0.0, pitch=0.0, roll=0.0):
    """Translation then rotations: yaw about Z, pitch about Y, roll about X (degrees)."""
    return (Matrix.Translation((x, y, z)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z') @ Matrix.Rotation(math.radians(pitch), 4, 'Y')
            @ Matrix.Rotation(math.radians(roll), 4, 'X'))


def xf(obj, matrix):
    obj.data.transform(matrix)
    return obj


def quad(name, corners, uv=((0, 0), (1, 0), (1, 1), (0, 1))):
    """One quad through four 3D corners (counter-clockwise seen from its front), with UVs."""
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    f = bm.faces.new([bm.verts.new(c) for c in corners])
    for loop, t in zip(f.loops, uv):
        loop[uvl].uv = t
    return core.mesh_object(name, bm)


def grid_plane(name, origin, u, v, w, h, nu, nv, disp=None):
    """A plane w x h through origin (its corner) spanned by u, v, split nu x nv; disp(s, t) -> offset along the normal.
    UVs 0..1 across it."""
    U, V = Vector(u).normalized(), Vector(v).normalized()
    N = U.cross(V).normalized()
    O = Vector(origin)
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    grid = []
    for j in range(nv + 1):
        row = []
        for i in range(nu + 1):
            s, t = i / nu, j / nv
            d = disp(s, t) if disp else 0.0
            row.append(bm.verts.new(O + U * (s * w) + V * (t * h) + N * d))
        grid.append(row)
    for j in range(nv):
        for i in range(nu):
            f = bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
            for loop, (a, b) in zip(f.loops, ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))):
                loop[uvl].uv = (a / nu, b / nv)
    return core.mesh_object(name, bm)


def wall_with_holes(name, x0, x1, z0, z1, holes, y, thick, toward=-1.0):
    """A wall slab in the XZ plane from x0..x1, z0..z1 with rectangular holes [(hx0, hx1, hz0, hz1)], its face at y, thick
    toward -toward*Y (the face looks toward `toward` * Y)."""
    outer = [(x0, z0), (x1, z0), (x1, z1), (x0, z1)]
    hl = [[(a, c), (a, d), (b, d), (b, c)] for a, b, c, d in holes]
    obj = parts.outline_prism(name, outer, hl, 0.0, thick)
    # outline_prism lays it in XY, extruded along Z: turn it into XZ, extruded along -toward*Y from y
    rot = Matrix.Rotation(math.radians(90.0), 4, 'X')   # (x, y, z) -> (x, -z, y)
    obj.data.transform(rot)
    # after the turn the slab spans Y -thick..0; place its face at y looking toward `toward`
    if toward < 0:
        obj.data.transform(Matrix.Translation((0.0, y + thick, 0.0)))
    else:
        obj.data.transform(Matrix.Translation((0.0, y, 0.0)))
    return obj


def blob(name, center, size, seed=1, detail=0.18, subdiv=3, flatten=0.0, wrinkle=0.0, smooth=0):
    """A soft lump: an icosphere scaled to size and pushed about by noise."""
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=1.0)
    P = np.array([v.co[:] for v in bm.verts])
    n = texmaps.value_noise(P * 0.5 + seed * 3.1, 0.6, seed=seed, octaves=3) - 0.5
    n2 = texmaps.value_noise(P * 1.3 + seed * 1.7, 0.35, seed=seed + 7, octaves=2) - 0.5
    n3 = np.abs(texmaps.value_noise(P * 2.2 + seed * 0.7, 0.3, seed=seed + 13, octaves=2) - 0.5) * 2.0
    for v, a, b, c in zip(bm.verts, n, n2, n3):
        d = v.co.normalized()
        k = 1.0 + detail * a * 2.0 + detail * 0.6 * b - wrinkle * (1.0 - c) ** 3
        co = d * k
        if flatten > 0.0 and co.z < 0.0:
            co.z *= (1.0 - flatten)
        v.co = Vector((center[0] + co.x * size[0] / 2, center[1] + co.y * size[1] / 2, center[2] + co.z * size[2] / 2))
    for _ in range(smooth):
        bmesh.ops.smooth_vert(bm, verts=bm.verts[:], factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    obj = core.mesh_object(name, bm)
    obj.data.shade_smooth()
    return obj


def soft_slab(name, w, d, h, r, nx=8, ny=8, nz=2, sag=0.0, seed=0, noise_amt=0.0, matrix=None):
    """A pillowy box (a cushion, a folded garment, a pad): a subdivided cube rounded by r, its top sagging by `sag` in
    the middle, pushed about by noise_amt. Origin at the bottom middle."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=max(nx, ny, nz) - 1, use_grid_fill=True)
    for v in bm.verts:
        x, y, z = v.co
        v.co = Vector((x * w, y * d, (z + 0.5) * h))
    obj = core.mesh_object(name, bm)
    me = obj.data
    P = np.array([v.co[:] for v in me.vertices])
    if r > 0:
        # round the box: move each vertex toward the inner box and back out by r along the offset direction
        hw, hd, hh = w / 2 - r, d / 2 - r, h / 2 - r
        c = P.copy()
        c[:, 2] -= h / 2
        inner = np.stack([np.clip(c[:, 0], -max(hw, 0), max(hw, 0)), np.clip(c[:, 1], -max(hd, 0), max(hd, 0)), np.clip(c[:, 2], -max(hh, 0), max(hh, 0))], 1)
        off = c - inner
        ln = np.linalg.norm(off, axis=1, keepdims=True)
        off = np.where(ln > 1e-9, off / np.maximum(ln, 1e-9) * r, off)
        P = inner + off
        P[:, 2] += h / 2
    if sag:
        rr = (P[:, 0] / (w / 2)) ** 2 + (P[:, 1] / (d / 2)) ** 2
        P[:, 2] -= sag * np.clip(1.0 - rr, 0, 1) * (P[:, 2] / max(h, 1e-6))
    if noise_amt:
        n = texmaps.value_noise(P * 1.0 + seed * 1.37, 0.08, seed=seed, octaves=3) - 0.5
        P[:, 2] += n * noise_amt * np.clip(P[:, 2] / max(h, 1e-6), 0, 1)
        n2 = texmaps.value_noise(P * 1.0 + seed * 2.11, 0.12, seed=seed + 5, octaves=2) - 0.5
        P[:, 0] += n2 * noise_amt * 0.6
    for v, p in zip(me.vertices, P):
        v.co = p
    if matrix is not None:
        me.transform(matrix)
    me.shade_smooth()
    return obj


def text_mesh(name, txt, size, face='Black', depth=0.01, align='CENTER', tracking=1.0, bevel=0.0, resolution=4):
    """Extruded letters (channel letters, cast numbers): a mesh, the text's baseline on Y 0 in the XY plane, extruded +Z."""
    cu = bpy.data.curves.new(name, 'FONT')
    cu.body = txt
    cu.font = bpy.data.fonts.load(os.path.join(core.FONTS, f'Roboto-{face}.ttf'), check_existing=True)
    cu.size = size
    cu.align_x = align
    cu.space_character = tracking
    cu.extrude = depth / 2
    cu.bevel_depth = bevel
    cu.bevel_resolution = 2 if bevel > 0 else 0
    cu.resolution_u = resolution
    ob = core.link(bpy.data.objects.new(name, cu))
    ob.location.z = depth / 2
    core.select_only([ob])
    bpy.ops.object.convert(target='MESH')
    obj = bpy.context.view_layer.objects.active
    core.apply_transforms(obj)
    obj.name = name
    obj.data.name = name
    weld(obj, 1e-6)
    core.orient_normals(obj)
    return obj


# ================================================================== the kit: parts, finishes, decals

# Finishes, chosen per face by the ns_fin attribute (ns_col tints those that take a color; ns_dir picks a wood grain's
# axis: 0 X, 1 Y, 2 Z; ns_var a variant or seed in 0..1).
FIN_NAMES = ['steel', 'chrome', 'alu', 'bronze', 'powder', 'paint', 'wallpaint', 'brickpaint', 'wood', 'laminate', 'particle',
             'plywood', 'carpet', 'cloth', 'vinyl', 'leather', 'rubber', 'plastic', 'gloss', 'paper', 'cardboard', 'cork',
             'blackglass', 'enamel', 'velvet', 'brass', 'foam', 'ceiltile', 'vct', 'rubberfloor', 'mosaic', 'concrete',
             'slatwall', 'pegboard', 'galv', 'chalk', 'glassbaked', 'prism', 'felt', 'canvas', 'lino', 'knurl', 'corrugated']
FIN = {n: i + 1 for i, n in enumerate(FIN_NAMES)}
METAL_FINS = {'steel', 'chrome', 'alu', 'bronze', 'brass', 'galv'}


class Kit:
    """One mesh's parts. Baked parts carry a finish (ns_fin) and a tint (ns_col) and belong to a bake group (each group
    is one baked slot); decals carry print-atlas UVs (the print slot); unbaked parts carry a shared unbaked material."""

    def __init__(self, prefix, name, sizes=None, dust=0.3, low_grime=0.12, module=None, groups=None):
        self.prefix, self.name = prefix, name
        self.sizes = sizes or {}
        self.module = module or MODULE
        self.objs = []
        self.kinds = {}             # group -> set of finishes
        self.group_order = list(groups or [])
        self.atlas = None
        self.dust = dust
        self.low = low_grime        # how high floor grime climbs (meters)
        self.grime = []             # (center, radius, amount): hand grime
        self.polish = []            # (center, radius, amount): worn glossy where hands rub or people sit
        self.params = {}            # finish-specific parameters (floors' layouts, logos, paths)
        self.cavity = 0.012         # the cavity-grime AO distance (meters)
        self.cavity_amt = 0.35
        self.pending = []           # decals placed before the atlas is rendered (it renders once, in finish)

    def add(self, obj, fin, col=0xffffff, group='body', dir=0, var=None, keep_uv=False):
        """keep_uv: the part keeps its own UVs (a floor laid 0..1 across its slot's whole texture)."""
        set_attr(obj, 'ns_fin', float(FIN[fin]))
        set_attr(obj, 'ns_col', lin3(col))
        set_attr(obj, 'ns_dir', float(dir))
        set_attr(obj, 'ns_var', float(random.random() if var is None else var))
        set_attr(obj, 'ns_lock', 1.0 if keep_uv else 0.0)
        obj['ns_slot'] = group
        self.kinds.setdefault(group, set()).add(fin)
        if group not in self.group_order:
            self.group_order.append(group)
        self.objs.append(obj)
        return obj

    def special(self, obj, mat, locked=True):
        """A part in an unbaked slot (glass, mirror, screen, belt): mat is its bpy material. locked keeps its own UVs."""
        set_attr(obj, 'ns_fin', 0.0)
        set_attr(obj, 'ns_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'ns_dir', 0.0)
        set_attr(obj, 'ns_var', 0.0)
        set_attr(obj, 'ns_lock', 1.0 if locked else 0.0)
        obj['ns_slot'] = mat.name
        obj['ns_unbaked'] = True
        self.objs.append(obj)
        return obj

    def panel(self, key, w, h, draw, glow=0.0, density=1.0):
        """A print panel w x h meters (drawn in millimeters)."""
        if self.atlas is None:
            self.atlas = Atlas(f'{self.prefix}_print', self.sizes.get(f'{self.prefix}_print', 1024))
            # a mesh that will reuse its last bake only needs the atlas's layout, not its picture
            self.atlas.skip_render = self.name in {s.strip() for s in os.environ.get('NS_REUSE', '').split(',')}
        self.atlas.add(key, w * 1000.0, h * 1000.0, draw, glow, density)

    def decal(self, key, origin, u=(1, 0, 0), v=(0, 0, 1), w=None, h=None, lift=0.0004, outline=None, nu=1, nv=1, disp=None,
              matrix=None, back=None):
        """Panel `key` laid on the plane through origin spanned by u (the print's right) and v (its up), facing u x v.
        outline: (s, t) points in [0, 1]^2 of the panel for a shaped decal; nu x nv: a grid, displaced along the
        normal by disp(s, t) (curled paper). back: a panel key printed on the reverse (a card's back), or None."""
        if self.atlas.color is None:
            self.pending.append(('decal', (key, origin), dict(u=u, v=v, w=w, h=h, lift=lift, outline=outline, nu=nu, nv=nv, disp=disp,
                                                                matrix=matrix, back=back)))
            return None
        p = self.atlas.panels[key]
        w = p['w'] / 1000.0 if w is None else w
        h = p['h'] / 1000.0 if h is None else h
        U, V = Vector(u).normalized(), Vector(v).normalized()
        N = U.cross(V).normalized()
        O = Vector(origin) + N * lift
        objs = [self._decal_face(key, O, U, V, N, w, h, outline, nu, nv, disp, matrix, False)]
        if back is not None:
            objs.append(self._decal_face(back, O - N * (lift * 0.5), U, V, N, w, h, outline, nu, nv, disp, matrix, True))
        return objs[0]

    def _decal_face(self, key, O, U, V, N, w, h, outline, nu, nv, disp, matrix, flip):
        u0, v0, u1, v1 = self.atlas.rect(key)
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new('UVMap')

        def pos(s, t):
            d = disp(s, t) if disp else 0.0
            return O + U * ((s - 0.5) * w) + V * ((t - 0.5) * h) + N * d

        def uvof(s, t):
            # the back reads correctly from behind: its s runs the other way
            return (u0 + (u1 - u0) * ((1 - s) if flip else s), v0 + (v1 - v0) * t)

        if outline:
            vs = [bm.verts.new(pos(s, t)) for s, t in outline]
            f = bm.faces.new(vs)
            for loop, (s, t) in zip(f.loops, outline):
                loop[uvl].uv = uvof(s, t)
        else:
            grid = [[bm.verts.new(pos(i / nu, j / nv)) for i in range(nu + 1)] for j in range(nv + 1)]
            for j in range(nv):
                for i in range(nu):
                    f = bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
                    for loop, (a, b) in zip(f.loops, ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))):
                        loop[uvl].uv = uvof(a / nu, b / nv)
        want = -N if flip else N
        for f in bm.faces:
            f.normal_update()
            if f.normal.dot(want) < 0:
                f.normal_flip()
        obj = core.mesh_object(f'decal_{key}', bm)
        if matrix is not None:
            obj.data.transform(matrix)
        set_attr(obj, 'ns_fin', 0.0)
        set_attr(obj, 'ns_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'ns_dir', 0.0)
        set_attr(obj, 'ns_var', 0.0)
        set_attr(obj, 'ns_lock', 1.0)
        obj['ns_slot'] = 'print'
        self.objs.append(obj)
        return obj

    def uv_print(self, obj, key):
        """A mesh printed with panel `key` through its own UVs (a lathe's: u round it, v up), squeezed into the panel."""
        if self.atlas.color is None:
            self.pending.append(('uv', (obj, key), {}))
            obj['ns_slot'] = 'pending'
            return obj
        u0, v0, u1, v1 = self.atlas.rect(key)
        uv = obj.data.uv_layers.active
        for d in uv.data:
            s_, t_ = d.uv
            d.uv = (u0 + (u1 - u0) * s_, v0 + (v1 - v0) * t_)
        set_attr(obj, 'ns_fin', 0.0)
        set_attr(obj, 'ns_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'ns_dir', 0.0)
        set_attr(obj, 'ns_var', 0.0)
        set_attr(obj, 'ns_lock', 1.0)
        obj['ns_slot'] = 'print'
        self.objs.append(obj)
        return obj

    def locked_print(self, obj, key, uvfun):
        """Any mesh printed with panel `key`: uvfun(co) -> (s, t) in [0, 1]^2 of the panel."""
        if self.atlas.color is None:
            self.pending.append(('locked', (obj, key, uvfun), {}))
            obj['ns_slot'] = 'pending'
            return obj
        u0, v0, u1, v1 = self.atlas.rect(key)
        me = obj.data
        uv = me.uv_layers.get('UVMap') or me.uv_layers.new(name='UVMap')
        for poly in me.polygons:
            for li in poly.loop_indices:
                s, t = uvfun(me.vertices[me.loops[li].vertex_index].co)
                uv.data[li].uv = (u0 + (u1 - u0) * s, v0 + (v1 - v0) * t)
        set_attr(obj, 'ns_fin', 0.0)
        set_attr(obj, 'ns_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'ns_dir', 0.0)
        set_attr(obj, 'ns_var', 0.0)
        set_attr(obj, 'ns_lock', 1.0)
        obj['ns_slot'] = 'print'
        self.objs.append(obj)
        return obj

    def finish(self, angle=35.0, name=None, unbaked_order=None):
        """Joins the parts into one mesh: its baked groups (in group_order), then the print slot, then the unbaked
        slots (in unbaked_order, else first use). Lays out UVs (decals and unbaked parts keep theirs) and prints the
        slot order."""
        name = name or self.name
        if self.atlas is not None and self.atlas.color is None:
            self.atlas.render()
        for kind, args, kw in self.pending:
            if kind == 'decal':
                self.decal(*args, **kw)
            elif kind == 'uv':
                self.uv_print(*args)
            else:
                self.locked_print(*args)
        self.pending = []
        mats = []
        by = {}
        for g in self.group_order:
            if any(o['ns_slot'] == g for o in self.objs):
                m = body_material(self, g)
                by[g] = m.m
                mats.append(m.m)
        if any(o['ns_slot'] == 'print' for o in self.objs):
            pm = print_material(self)
            by['print'] = pm.m
            mats.append(pm.m)
        unb = []
        for o in self.objs:
            if o.get('ns_unbaked') and o['ns_slot'] not in unb:
                unb.append(o['ns_slot'])
        if unbaked_order:
            unb = [n for n in unbaked_order if n in unb] + [n for n in unb if n not in unbaked_order]
        for n in unb:
            by[n] = bpy.data.materials[n]
            mats.append(by[n])
        for o in self.objs:
            core.assign(o, by[o['ns_slot']])
            if not o.data.uv_layers:
                o.data.uv_layers.new(name='UVMap')
            elif o.data.uv_layers[0].name != 'UVMap':
                o.data.uv_layers[0].name = 'UVMap'
        obj = core.join(self.objs, name) if len(self.objs) > 1 else self.objs[0]
        obj.name = name
        obj.data.name = name
        order_slots(obj, mats)
        core.finish_hard_surface(obj, angle)
        lock = obj.data.attributes.get('ns_lock')
        vals = np.zeros(len(obj.data.polygons), dtype=np.float32)
        lock.data.foreach_get('value', vals)
        locked = set(np.nonzero(vals > 0.5)[0].tolist())
        groups = [(lambda f: f.index in locked, 'keep', None, (0.0, 0.0, 1.0, 1.0))]
        for s in obj.material_slots:
            groups.append((core.material_is(obj, s.material.name), 'box', None, (0.0, 0.0, 1.0, 1.0)))
        groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
        core.uv_layout(obj, groups)
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        print(f'[{self.module}] {name}: {tris} triangles, slots {[s.material.name for s in obj.material_slots]}')
        obj['ns_kit'] = self.prefix
        return obj


def order_slots(obj, mats):
    """Puts obj's material slots in the order of mats and remaps the faces."""
    me = obj.data
    old = [s.material for s in obj.material_slots]
    remap = [mats.index(mt) for mt in old]
    fi = np.zeros(len(me.polygons), dtype=np.int32)
    me.polygons.foreach_get('material_index', fi)
    new = np.array(remap, dtype=np.int32)[fi] if remap else fi
    me.materials.clear()
    for mt in mats:
        me.materials.append(mt)
    me.polygons.foreach_set('material_index', new)


def glow(objs, name, module=None):
    """A _Glow<Family> mesh (spec 0.5): one white emissive slot the game colors by the hour."""
    objs = [o for o in (objs if isinstance(objs, (list, tuple)) else [objs])]
    for o in objs:
        for a in list(o.data.attributes):
            if a.name.startswith('ns_'):
                o.data.attributes.remove(a)
    obj = parts.glow_object(objs, name)
    obj['ns_unbaked'] = True
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    print(f'[{module or MODULE}] {name}: {tris} triangles, slots {[s.material.name for s in obj.material_slots]}')
    return obj


# ------------------------------------------------------------------ print atlas

class Sub:
    """A panel's drawing space inside an atlas Sheet: millimeters from the panel's corner."""

    def __init__(self, s, ox, oy, k):
        self.s, self.ox, self.oy, self.k = s, ox, oy, k

    def P(self, x, y):
        return (self.ox + x * self.k, self.oy + y * self.k)

    def rect(self, x, y, w, h, color, metal=0.0, rough=0.4, rot=0.0, alpha=1.0):
        X, Y = self.P(x, y)
        return self.s.rect(X, Y, w * self.k, h * self.k, color, metal, rough, rot, alpha)

    def poly(self, pts, color, metal=0.0, rough=0.4, alpha=1.0):
        return self.s.poly([self.P(x, y) for x, y in pts], color, metal, rough, alpha)

    def circle(self, cx, cy, r, color, metal=0.0, rough=0.4, n=48, alpha=1.0):
        X, Y = self.P(cx, cy)
        return self.s.circle(X, Y, r * self.k, color, metal, rough, n, alpha)

    def ring(self, cx, cy, r0, r1, color, metal=0.0, rough=0.4, n=64, a0=0.0, a1=2 * math.pi, alpha=1.0):
        X, Y = self.P(cx, cy)
        return self.s.ring(X, Y, r0 * self.k, r1 * self.k, color, metal, rough, n, a0, a1, alpha)

    def text(self, txt, x, y, size, color, face='Black', align='LEFT', rot=0.0, tracking=1.0, metal=0.0, rough=0.4, alpha=1.0):
        X, Y = self.P(x, y)
        return self.s.text(txt, X, Y, size * self.k, color, face=face, align=align, rot=rot, tracking=tracking, metal=metal, rough=rough,
                           alpha=alpha)

    def fit(self, txt, x, y, size, max_w, color, face='Black', align='CENTER', tracking=1.0, metal=0.0, rough=0.4, rot=0.0):
        size = min(size, max_w / text_w(self.s, txt, face, tracking))
        self.text(txt, x, y, size, color, face, align, rot, tracking, metal, rough)
        return size

    def mid(self, txt, x, cy, size, max_w, color, face='Black', align='CENTER', tracking=1.0, metal=0.0, rough=0.4, alpha=1.0):
        size = min(size, max_w / text_w(self.s, txt, face, tracking))
        self.text(txt, x, cy - 0.355 * size, size, color, face, align, 0.0, tracking, metal, rough, alpha)
        return size

    def mesh(self, verts, faces, color, metal=0.0, rough=0.5, alpha=1.0):
        me = bpy.data.meshes.new('poly')
        me.from_pydata([(self.P(x, y)[0], self.P(x, y)[1], 0.0) for x, y in verts], [], faces)
        return self.s._add(bpy.data.objects.new('poly', me), color, metal, rough, alpha)

    def stroke(self, pts, width, color, rough=0.6, alpha=1.0):
        """A pen or brush line through pts (rounded joints)."""
        verts, faces = [], []
        hw = width / 2
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            dx, dy = x1 - x0, y1 - y0
            ln = math.hypot(dx, dy)
            if ln < 1e-9:
                continue
            nx, ny = -dy / ln * hw, dx / ln * hw
            b = len(verts)
            verts += [(x0 + nx, y0 + ny), (x0 - nx, y0 - ny), (x1 - nx, y1 - ny), (x1 + nx, y1 + ny)]
            faces.append((b, b + 1, b + 2, b + 3))
        for x, y in pts:
            b = len(verts)
            verts += [(x + hw * math.cos(2 * math.pi * k / 10), y + hw * math.sin(2 * math.pi * k / 10)) for k in range(10)]
            faces.append(tuple(range(b, b + 10)))
        if faces:
            self.mesh(verts, faces, color, 0.0, rough, alpha)


_WIDTHS = {}


def text_w(s, text, face='Black', tracking=1.0):
    """Width of text at size 1 (cached)."""
    key = (text, face, tracking)
    if key not in _WIDTHS:
        cu = bpy.data.curves.new('m', 'FONT')
        cu.body = text
        cu.font = s._font(face)
        cu.size = 1.0
        cu.space_character = tracking
        ob = bpy.data.objects.new('m', cu)
        core.link(ob)
        bpy.context.view_layer.update()
        _WIDTHS[key] = max(ob.dimensions.x, 1e-3)
        bpy.data.objects.remove(ob)
        bpy.data.curves.remove(cu)
    return _WIDTHS[key]


class Atlas:
    """Print panels (millimeters) packed into one square image, rendered in one pass."""

    def __init__(self, name, px, gap=0.006):
        self.name, self.px, self.gap = name, px, gap
        self.panels = {}
        self.rects = {}
        self.color = self.surface = None
        self.skip_render = False

    def add(self, key, w, h, draw, glow=0.0, density=1.0):
        self.panels[key] = {'w': w, 'h': h, 'draw': draw, 'glow': glow, 'd': density}

    def _pack(self, s):
        order = sorted(self.panels.items(), key=lambda kv: -kv[1]['h'] * kv[1]['d'])
        x = y = shelf = 0.0
        pos = {}
        for k, p in order:
            w, h = p['w'] * p['d'] * s, p['h'] * p['d'] * s
            if w > 1.0:
                return None
            if x + w > 1.0:
                y += shelf + self.gap
                x = shelf = 0.0
            if y + h > 1.0:
                return None
            pos[k] = (x, y, w, h)
            x += w + self.gap
            shelf = max(shelf, h)
        return pos

    def render(self):
        lo, hi = 0.0, 1e-6
        while self._pack(hi) is not None:
            hi *= 2.0
        for _ in range(40):
            mid = (lo + hi) / 2
            if self._pack(mid) is not None:
                lo = mid
            else:
                hi = mid
        pos = self._pack(lo)
        if self.skip_render:
            out_dir = os.path.join(core.BUILD_DIR, 'textures')
            self.color = os.path.join(out_dir, f'{self.name}_color.png')
            self.surface = os.path.join(out_dir, f'{self.name}_surface.png')
            if os.path.exists(self.color) and os.path.exists(self.surface):
                for k_, (x, y, w, h) in pos.items():
                    self.rects[k_] = (x, y, x + w, y + h)
                print(f'[{MODULE}] atlas {self.name}: layout only (its mesh reuses its bake)')
                return self
        s = Sheet(1000.0, 1000.0)
        for k, (x, y, w, h) in pos.items():
            p = self.panels[k]
            p['draw'](Sub(s, x * 1000.0, y * 1000.0, w * 1000.0 / p['w']), p['w'], p['h'])
            self.rects[k] = (x, y, x + w, y + h)
        self.color, self.surface = s.render(self.name, self.px, samples=10)
        for obj, *_ in s.items:
            data = obj.data
            bpy.data.objects.remove(obj)
            if isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)
            elif data is not None:
                bpy.data.curves.remove(data)
        for o in [o for o in bpy.data.objects if o.name.startswith('sheet_cam')]:
            bpy.data.objects.remove(o)
        for mt in [mt for mt in bpy.data.materials if mt.name.startswith('sheet') and mt.users == 0]:
            bpy.data.materials.remove(mt)
        print(f'[{MODULE}] atlas {self.name}: {len(self.panels)} panels, {lo * self.px:.2f} px/mm at density 1')
        return self

    def rect(self, key):
        """The panel's UV rect, inset half a texel."""
        x0, y0, x1, y1 = self.rects[key]
        e = 0.6 / self.px
        return (x0 + e, y0 + e, x1 - e, y1 - e)


def render_sheet(s, name, px, samples=16):
    """Renders a standalone Sheet and removes its objects; returns the color image's path."""
    color, _surface = s.render(name, px, samples=samples)
    for obj, *_ in s.items:
        data = obj.data
        bpy.data.objects.remove(obj)
        if isinstance(data, bpy.types.Mesh):
            bpy.data.meshes.remove(data)
        elif data is not None:
            bpy.data.curves.remove(data)
    for o in [o for o in bpy.data.objects if o.name.startswith('sheet_cam')]:
        bpy.data.objects.remove(o)
    return color


# ------------------------------------------------------------------ handwriting: a marker and a pen

# Strokes per glyph on a cap height of 1.0 (x, y), and the advance. Hand-lettered signs, tags and the bulletin board.
_CAPS = {
    'A': ([[(0.0, 0.0), (0.3, 1.0), (0.6, 0.0)], [(0.12, 0.38), (0.48, 0.38)]], 0.64),
    'B': ([[(0.06, 0.0), (0.06, 1.0), (0.34, 1.0), (0.48, 0.93), (0.51, 0.79), (0.44, 0.61), (0.28, 0.54), (0.06, 0.54)],
           [(0.28, 0.54), (0.47, 0.48), (0.55, 0.32), (0.52, 0.12), (0.38, 0.01), (0.06, 0.0)]], 0.62),
    'C': ([[(0.56, 0.82), (0.46, 0.96), (0.3, 1.0), (0.14, 0.92), (0.04, 0.7), (0.04, 0.3), (0.14, 0.08), (0.3, 0.0), (0.46, 0.04),
            (0.57, 0.18)]], 0.62),
    'D': ([[(0.06, 0.0), (0.06, 1.0), (0.28, 0.98), (0.48, 0.86), (0.57, 0.5), (0.49, 0.15), (0.28, 0.02), (0.06, 0.0)]], 0.64),
    'E': ([[(0.52, 1.0), (0.06, 1.0), (0.06, 0.0), (0.52, 0.0)], [(0.06, 0.52), (0.42, 0.52)]], 0.58),
    'F': ([[(0.52, 1.0), (0.06, 1.0), (0.06, 0.0)], [(0.06, 0.52), (0.42, 0.52)]], 0.56),
    'G': ([[(0.56, 0.82), (0.46, 0.96), (0.3, 1.0), (0.14, 0.92), (0.04, 0.7), (0.04, 0.3), (0.14, 0.08), (0.3, 0.0), (0.48, 0.04),
            (0.58, 0.2), (0.58, 0.44), (0.36, 0.44)]], 0.66),
    'H': ([[(0.06, 1.0), (0.06, 0.0)], [(0.54, 1.0), (0.54, 0.0)], [(0.06, 0.52), (0.54, 0.52)]], 0.66),
    'I': ([[(0.1, 1.0), (0.1, 0.0)]], 0.26),
    'J': ([[(0.44, 1.0), (0.44, 0.24), (0.36, 0.06), (0.22, 0.0), (0.08, 0.06), (0.02, 0.2)]], 0.54),
    'K': ([[(0.06, 1.0), (0.06, 0.0)], [(0.52, 1.0), (0.06, 0.4)], [(0.2, 0.54), (0.56, 0.0)]], 0.6),
    'L': ([[(0.06, 1.0), (0.06, 0.0), (0.5, 0.0)]], 0.54),
    'M': ([[(0.06, 0.0), (0.08, 1.0), (0.36, 0.3), (0.64, 1.0), (0.66, 0.0)]], 0.76),
    'N': ([[(0.06, 0.0), (0.06, 1.0), (0.54, 0.0), (0.54, 1.0)]], 0.66),
    'O': ([[(0.32, 1.0), (0.12, 0.92), (0.03, 0.7), (0.03, 0.3), (0.12, 0.08), (0.32, 0.0), (0.52, 0.08), (0.61, 0.3), (0.61, 0.7),
            (0.52, 0.92), (0.32, 1.0), (0.26, 0.98)]], 0.68),
    'P': ([[(0.06, 0.0), (0.06, 1.0), (0.36, 1.0), (0.52, 0.9), (0.54, 0.72), (0.46, 0.56), (0.32, 0.5), (0.06, 0.5)]], 0.6),
    'Q': ([[(0.32, 1.0), (0.12, 0.92), (0.03, 0.7), (0.03, 0.3), (0.12, 0.08), (0.32, 0.0), (0.52, 0.08), (0.61, 0.3), (0.61, 0.7),
            (0.52, 0.92), (0.32, 1.0), (0.26, 0.98)], [(0.38, 0.22), (0.62, -0.06)]], 0.68),
    'R': ([[(0.06, 0.0), (0.06, 1.0), (0.36, 1.0), (0.52, 0.9), (0.54, 0.72), (0.46, 0.56), (0.32, 0.5), (0.06, 0.5)],
           [(0.3, 0.5), (0.56, 0.0)]], 0.62),
    'S': ([[(0.52, 0.86), (0.4, 0.98), (0.22, 1.0), (0.08, 0.9), (0.06, 0.72), (0.18, 0.58), (0.4, 0.46), (0.54, 0.32), (0.54, 0.14),
            (0.4, 0.02), (0.2, 0.0), (0.04, 0.12)]], 0.6),
    'T': ([[(0.0, 1.0), (0.6, 1.0)], [(0.3, 1.0), (0.3, 0.0)]], 0.62),
    'U': ([[(0.06, 1.0), (0.06, 0.3), (0.14, 0.08), (0.32, 0.0), (0.5, 0.08), (0.58, 0.3), (0.58, 1.0)]], 0.66),
    'V': ([[(0.0, 1.0), (0.3, 0.0), (0.6, 1.0)]], 0.62),
    'W': ([[(0.0, 1.0), (0.2, 0.0), (0.4, 0.8), (0.6, 0.0), (0.8, 1.0)]], 0.84),
    'X': ([[(0.04, 1.0), (0.56, 0.0)], [(0.56, 1.0), (0.04, 0.0)]], 0.62),
    'Y': ([[(0.0, 1.0), (0.3, 0.5), (0.6, 1.0)], [(0.3, 0.5), (0.3, 0.0)]], 0.62),
    'Z': ([[(0.04, 1.0), (0.56, 1.0), (0.04, 0.0), (0.58, 0.0)]], 0.62),
    '0': ([[(0.28, 1.0), (0.1, 0.9), (0.03, 0.6), (0.04, 0.3), (0.12, 0.06), (0.28, 0.0), (0.44, 0.06), (0.52, 0.3), (0.53, 0.6),
            (0.46, 0.9), (0.28, 1.0), (0.22, 0.97)]], 0.58),
    '1': ([[(0.08, 0.8), (0.26, 1.0), (0.26, 0.0)]], 0.42),
    '2': ([[(0.06, 0.8), (0.16, 0.96), (0.32, 1.0), (0.48, 0.92), (0.54, 0.76), (0.48, 0.58), (0.06, 0.0), (0.56, 0.0)]], 0.6),
    '3': ([[(0.06, 0.88), (0.2, 1.0), (0.4, 0.98), (0.52, 0.86), (0.5, 0.68), (0.32, 0.56), (0.24, 0.55)],
           [(0.32, 0.56), (0.5, 0.46), (0.56, 0.26), (0.48, 0.08), (0.3, 0.0), (0.12, 0.04), (0.04, 0.14)]], 0.6),
    '4': ([[(0.42, 0.0), (0.42, 1.0), (0.02, 0.32), (0.58, 0.32)]], 0.62),
    '5': ([[(0.52, 1.0), (0.12, 1.0), (0.08, 0.56), (0.26, 0.62), (0.44, 0.56), (0.54, 0.38), (0.5, 0.12), (0.32, 0.0), (0.12, 0.02),
            (0.04, 0.12)]], 0.6),
    '6': ([[(0.5, 0.92), (0.36, 1.0), (0.2, 0.96), (0.08, 0.76), (0.04, 0.42), (0.08, 0.14), (0.24, 0.0), (0.42, 0.04), (0.54, 0.2),
            (0.52, 0.42), (0.38, 0.56), (0.2, 0.54), (0.06, 0.4)]], 0.6),
    '7': ([[(0.04, 1.0), (0.56, 1.0), (0.24, 0.0)]], 0.58),
    '8': ([[(0.3, 0.54), (0.12, 0.64), (0.08, 0.82), (0.18, 0.96), (0.3, 1.0), (0.42, 0.96), (0.52, 0.82), (0.48, 0.64), (0.3, 0.54),
            (0.1, 0.44), (0.04, 0.24), (0.12, 0.06), (0.3, 0.0), (0.48, 0.06), (0.56, 0.24), (0.5, 0.44), (0.3, 0.54)]], 0.6),
    '9': ([[(0.52, 0.6), (0.38, 0.46), (0.2, 0.46), (0.06, 0.6), (0.06, 0.84), (0.2, 1.0), (0.38, 1.0), (0.52, 0.86), (0.54, 0.56),
            (0.46, 0.18), (0.32, 0.0), (0.12, 0.04)]], 0.6),
    '.': ([[(0.08, 0.02), (0.09, 0.04)]], 0.24),
    ',': ([[(0.1, 0.06), (0.06, -0.14)]], 0.24),
    '!': ([[(0.1, 1.0), (0.1, 0.28)], [(0.1, 0.02), (0.11, 0.04)]], 0.26),
    '?': ([[(0.04, 0.82), (0.14, 0.96), (0.3, 1.0), (0.46, 0.94), (0.52, 0.78), (0.44, 0.6), (0.28, 0.48), (0.26, 0.28)],
           [(0.26, 0.02), (0.27, 0.04)]], 0.56),
    '-': ([[(0.04, 0.44), (0.36, 0.45)]], 0.42),
    "'": ([[(0.1, 1.0), (0.08, 0.76)]], 0.2),
    '/': ([[(0.0, -0.06), (0.4, 1.04)]], 0.44),
    '&': ([[(0.6, 0.0), (0.16, 0.6), (0.12, 0.82), (0.22, 0.98), (0.36, 0.96), (0.42, 0.82), (0.36, 0.66), (0.08, 0.42), (0.04, 0.2),
            (0.14, 0.04), (0.3, 0.0), (0.46, 0.1), (0.58, 0.32)]], 0.66),
    '$': ([[(0.5, 0.82), (0.38, 0.9), (0.2, 0.9), (0.08, 0.78), (0.12, 0.6), (0.32, 0.5), (0.48, 0.38), (0.5, 0.2), (0.38, 0.08),
            (0.18, 0.08), (0.04, 0.18)], [(0.28, 1.04), (0.28, -0.06)]], 0.56),
    ':': ([[(0.08, 0.6), (0.09, 0.62)], [(0.08, 0.04), (0.09, 0.06)]], 0.24),
    '#': ([[(0.18, 0.0), (0.26, 1.0)], [(0.4, 0.0), (0.48, 1.0)], [(0.04, 0.32), (0.6, 0.32)], [(0.06, 0.66), (0.62, 0.66)]], 0.66),
    '(': ([[(0.24, 1.04), (0.1, 0.8), (0.06, 0.5), (0.1, 0.2), (0.24, -0.04)]], 0.3),
    ')': ([[(0.06, 1.04), (0.2, 0.8), (0.24, 0.5), (0.2, 0.2), (0.06, -0.04)]], 0.3),
    '+': ([[(0.04, 0.46), (0.48, 0.46)], [(0.26, 0.24), (0.26, 0.68)]], 0.54),
    '*': ([[(0.24, 0.98), (0.24, 0.5)], [(0.04, 0.86), (0.44, 0.62)], [(0.44, 0.86), (0.04, 0.62)]], 0.5),
    '@': ([[(0.46, 0.5), (0.36, 0.62), (0.24, 0.58), (0.2, 0.42), (0.28, 0.3), (0.42, 0.34), (0.48, 0.6), (0.5, 0.3), (0.62, 0.32),
            (0.68, 0.5), (0.6, 0.78), (0.4, 0.9), (0.18, 0.82), (0.06, 0.56), (0.12, 0.2), (0.34, 0.06), (0.56, 0.12)]], 0.74),
}
# Lowercase (x-height 0.5, caps 0.72) for script lines: "St. Agnes Outreach", names, flyers.
_LOWER = {
    'a': ([[(0.38, 0.40), (0.28, 0.49), (0.14, 0.48), (0.04, 0.34), (0.04, 0.14), (0.12, 0.02), (0.26, 0.02), (0.38, 0.16)],
           [(0.39, 0.50), (0.39, 0.10), (0.46, 0.0)]], 0.52),
    'b': ([[(0.05, 0.78), (0.05, 0.36), (0.05, 0.0)], [(0.05, 0.26), (0.14, 0.44), (0.28, 0.50), (0.39, 0.40), (0.42, 0.22), (0.35, 0.05),
                                                     (0.21, 0.0), (0.05, 0.05)]], 0.48),
    'c': ([[(0.38, 0.42), (0.26, 0.50), (0.12, 0.46), (0.04, 0.30), (0.06, 0.12), (0.18, 0.0), (0.32, 0.02), (0.40, 0.10)]], 0.44),
    'd': ([[(0.38, 0.40), (0.28, 0.49), (0.14, 0.48), (0.04, 0.34), (0.04, 0.14), (0.12, 0.02), (0.26, 0.02), (0.38, 0.16)],
           [(0.40, 0.78), (0.40, 0.36), (0.40, 0.08), (0.46, 0.0)]], 0.50),
    'e': ([[(0.06, 0.25), (0.36, 0.28), (0.40, 0.38), (0.32, 0.48), (0.18, 0.50), (0.07, 0.40), (0.03, 0.22), (0.10, 0.06), (0.24, 0.0),
            (0.40, 0.06)]], 0.46),
    'f': ([[(0.36, 0.72), (0.26, 0.76), (0.16, 0.70), (0.13, 0.55), (0.13, 0.25), (0.12, 0.0)], [(0.0, 0.46), (0.30, 0.47)]], 0.34),
    'g': ([[(0.38, 0.40), (0.28, 0.49), (0.14, 0.48), (0.04, 0.34), (0.04, 0.16), (0.12, 0.04), (0.26, 0.04), (0.38, 0.18)],
           [(0.39, 0.50), (0.39, 0.10), (0.36, -0.14), (0.24, -0.24), (0.08, -0.20)]], 0.50),
    'h': ([[(0.05, 0.78), (0.05, 0.36), (0.05, 0.0)], [(0.05, 0.28), (0.14, 0.44), (0.28, 0.50), (0.38, 0.42), (0.40, 0.26), (0.40, 0.0)]], 0.50),
    'i': ([[(0.08, 0.48), (0.08, 0.25), (0.08, 0.02), (0.12, 0.0)], [(0.08, 0.66), (0.085, 0.67)]], 0.20),
    'j': ([[(0.18, 0.48), (0.18, 0.0), (0.14, -0.18), (0.02, -0.24)], [(0.18, 0.66), (0.185, 0.67)]], 0.26),
    'k': ([[(0.05, 0.76), (0.05, 0.36), (0.05, 0.0)], [(0.36, 0.50), (0.06, 0.22), (0.38, 0.0)]], 0.42),
    'l': ([[(0.08, 0.76), (0.08, 0.40), (0.08, 0.05), (0.13, 0.0)]], 0.20),
    'm': ([[(0.04, 0.50), (0.04, 0.25), (0.04, 0.0)], [(0.04, 0.30), (0.12, 0.46), (0.22, 0.50), (0.30, 0.42), (0.31, 0.0)],
           [(0.31, 0.32), (0.40, 0.47), (0.50, 0.50), (0.58, 0.42), (0.59, 0.0)]], 0.66),
    'n': ([[(0.05, 0.50), (0.05, 0.25), (0.05, 0.0)], [(0.05, 0.30), (0.14, 0.45), (0.28, 0.50), (0.38, 0.42), (0.40, 0.26), (0.40, 0.0)]], 0.48),
    'o': ([[(0.22, 0.50), (0.08, 0.43), (0.03, 0.25), (0.08, 0.07), (0.22, 0.0), (0.36, 0.07), (0.41, 0.25), (0.36, 0.43), (0.22, 0.50),
            (0.17, 0.48)]], 0.46),
    'p': ([[(0.05, 0.50), (0.05, 0.1), (0.05, -0.26)], [(0.05, 0.3), (0.14, 0.46), (0.28, 0.50), (0.39, 0.40), (0.41, 0.22), (0.34, 0.05),
                                                      (0.2, 0.0), (0.05, 0.06)]], 0.48),
    'r': ([[(0.05, 0.50), (0.05, 0.25), (0.05, 0.0)], [(0.05, 0.30), (0.12, 0.44), (0.24, 0.50), (0.34, 0.47)]], 0.36),
    's': ([[(0.36, 0.44), (0.24, 0.50), (0.10, 0.47), (0.06, 0.37), (0.15, 0.28), (0.30, 0.22), (0.38, 0.11), (0.29, 0.01), (0.13, 0.0),
            (0.03, 0.07)]], 0.44),
    't': ([[(0.16, 0.70), (0.16, 0.10), (0.22, 0.0), (0.32, 0.03)], [(0.02, 0.47), (0.18, 0.48), (0.34, 0.50)]], 0.38),
    'u': ([[(0.04, 0.50), (0.04, 0.16), (0.10, 0.03), (0.22, 0.0), (0.34, 0.06), (0.41, 0.22), (0.42, 0.5), (0.42, 0.22), (0.43, 0.04),
            (0.48, 0.0)]], 0.52),
    'v': ([[(0.02, 0.5), (0.2, 0.0), (0.38, 0.5)]], 0.42),
    'w': ([[(0.02, 0.5), (0.14, 0.0), (0.27, 0.4), (0.4, 0.0), (0.52, 0.5)]], 0.56),
    'x': ([[(0.04, 0.5), (0.38, 0.0)], [(0.38, 0.5), (0.04, 0.0)]], 0.44),
    'y': ([[(0.04, 0.50), (0.10, 0.24), (0.21, 0.02)], [(0.40, 0.50), (0.28, 0.16), (0.16, -0.14), (0.08, -0.24), (0.0, -0.25)]], 0.44),
    'z': ([[(0.04, 0.49), (0.20, 0.50), (0.36, 0.50), (0.20, 0.25), (0.04, 0.01), (0.22, 0.0), (0.38, 0.0)]], 0.42),
}


def _glyph(ch):
    if ch in _LOWER:
        return _LOWER[ch]
    g = _CAPS.get(ch.upper()) if ch not in _CAPS else _CAPS[ch]
    if g is None:
        return None
    strokes, adv = g
    return [[(x * 0.72, y * 0.72) for x, y in st] for st in strokes], adv * 0.72


def hand(s, text, x, y, size, color, width=0.075, slant=0.12, seed=1, smooth=4, wobble=0.012, alpha=1.0):
    """Handwritten text (marker, ballpoint, brush): baseline at y; size: a capital is 0.72 * size tall."""
    rnd = random.Random(seed)
    cx = x
    for ch in text:
        if ch == ' ':
            cx += 0.30 * size
            continue
        g = _glyph(ch)
        if g is None:
            cx += 0.4 * size
            continue
        strokes, adv = g
        js = rnd.uniform(0.94, 1.06)
        jx, jy = rnd.uniform(-0.03, 0.03), rnd.uniform(-0.04, 0.03)
        for st in strokes:
            pts = [(cx + (px * js + py * js * slant + jx + rnd.uniform(-wobble, wobble)) * size, y + (py * js + jy + rnd.uniform(-wobble, wobble)) * size)
                   for px, py in st]
            if len(pts) > 2:
                pts = [tuple(p) for p in core.catmull_rom(pts, smooth)]
            s.stroke(pts, width * size * rnd.uniform(0.9, 1.1), color, alpha=alpha)
        cx += (adv * js + 0.05) * size
    return cx


def hand_w(text, size):
    w = 0.0
    for ch in text:
        if ch == ' ':
            w += 0.30
            continue
        g = _glyph(ch)
        w += (g[1] if g else 0.4) + 0.05
    return w * size


def hand_mid(s, text, cx, cy, size, max_w, color, width=0.075, slant=0.12, seed=1, alpha=1.0):
    """Handwriting centered at (cx, cy) (cy: the middle of the capitals), shrunk to fit max_w."""
    size = min(size, max_w / max(hand_w(text, 1.0), 1e-6))
    hand(s, text, cx - hand_w(text, size) / 2, cy - 0.36 * size, size, color, width, slant, seed, alpha=alpha)
    return size


# ------------------------------------------------------------------ the unbaked slots the game drives

UNB = {}
UNBAKED_PREFIXES = ('ns_glass', 'ns_mirror', 'ns_screen', 'ns_belt', 'ns_runtime')


def make_unbaked(tag='TH', mirror_size=(0.45, 1.71)):
    """The shared unbaked materials (one per name, so the names never pick up a .001): ns_glass ([glass], spec 0.5),
    ns_mirror ([mirror]: its base color is the desilvering mask, UV 0..1 across each panel of mirror_size meters)."""
    UNB.clear()
    g = core.Mat('ns_glass')
    g.set('Base Color', H(0x26302f))
    g.set('Roughness', 0.03)
    g.set('Metallic', 0.0)
    g.set('Alpha', 0.2)
    g.set('Specular IOR Level', 0.6)
    g.m.use_backface_culling = False
    try:
        g.m.surface_render_method = 'BLENDED'
    except (AttributeError, TypeError):
        pass
    UNB['ns_glass'] = g.m
    mr = core.Mat('ns_mirror')
    img = mr.image(mirror_mask(tag, mirror_size))
    mr.link(img.outputs['Color'], mr.bsdf.inputs['Base Color'])
    mr.set('Metallic', 1.0)
    mr.set('Roughness', 0.02)
    mr.m.use_backface_culling = True
    UNB['ns_mirror'] = mr.m
    for n, mt in UNB.items():
        if mt.name != n:
            raise RuntimeError(f'material name clash: wanted {n}, got {mt.name}')


def screen_mat(name, picture, strength=1.6):
    """An unbaked [screen] slot (UV 0..1 across the screen as seen: u right, v up) showing `picture` until the game
    draws its own."""
    if name in UNB:
        return UNB[name]
    t = core.Mat(name)
    img = t.image(picture)
    t.set('Base Color', H(0x050607))
    t.link(img.outputs['Color'], t.bsdf.inputs['Emission Color'])
    t.set('Emission Strength', strength)
    t.set('Roughness', 0.2)
    t.m.use_backface_culling = True
    if t.m.name != name:
        raise RuntimeError(f'material name clash: wanted {name}, got {t.m.name}')
    UNB[name] = t.m
    return t.m


def image_mat(name, color_path, rough=0.5, metal=0.0, emission=0.0):
    """An unbaked slot shaded by one image (a runtime belt, a print the game pans)."""
    if name in UNB:
        return UNB[name]
    t = core.Mat(name)
    img = t.image(color_path)
    t.link(img.outputs['Color'], t.bsdf.inputs['Base Color'])
    t.set('Roughness', rough)
    t.set('Metallic', metal)
    if emission > 0.0:
        t.link(img.outputs['Color'], t.bsdf.inputs['Emission Color'])
        t.set('Emission Strength', emission)
    t.m.use_backface_culling = True
    UNB[name] = t.m
    return t.m


def mirror_mask(tag='TH', size=(0.45, 1.55)):
    """The [mirror]'s desilvering mask: silver (light) inside, cloudy grey-brown blooms creeping in a centimeter or
    three from the edges (more along the bottom, where cleaner runs down behind the glass), a few spots. Worked in
    meters on a panel of `size` (width, height), so a tall panel's edges aren't streaked by a stretched texture."""
    n = 512
    pw, ph = size
    u = (np.arange(n) + 0.5) / n
    U, V = np.meshgrid(u, u)
    X, Y = U * pw, V * ph
    edge = np.minimum(np.minimum(X, pw - X), np.minimum(Y, ph - Y))
    edge = np.minimum(edge, Y * 0.7)                      # the bottom edge goes first
    P = np.stack([X, Y, np.full_like(X, 0.37 * len(tag))], -1).reshape(-1, 3)
    w = texmaps.value_noise(P, 0.11, seed=7, octaves=4).reshape(n, n)
    reach = 0.004 + 0.028 * np.clip((w - 0.42) * 2.4, 0, 1) ** 1.5
    bloom = texmaps.smoothstep(reach, reach * 0.25, edge)
    cloud = texmaps.value_noise(P, 0.018, seed=19, octaves=3).reshape(n, n)
    bloom = bloom * np.clip(0.45 + 0.9 * (cloud - 0.3), 0.2, 1.0)
    sp = texmaps.value_noise(P, 0.012, seed=11, octaves=2).reshape(n, n)
    spots = texmaps.smoothstep(0.79, 0.84, sp) * 0.55 * texmaps.smoothstep(0.10, 0.03, edge)
    silver = np.clip(1.0 - 0.72 * bloom - spots, 0.0, 1.0)
    tone = 0.84 + 0.08 * silver
    dark = np.array([0.16, 0.14, 0.12])                   # a desilvered patch is grey-brown, not black
    rgb = silver[..., None] * tone[..., None] + (1 - silver[..., None]) * dark
    return texmaps.save(f'T_NS{tag}_MirrorMask', rgb, 'sRGB')


# ------------------------------------------------------------------ body finishes (shader)

class Ctx:
    def __init__(self, m, kit):
        self.m, self.kit = m, kit
        self.tc, self.sep = object_coords(m)
        self.P = self.tc.outputs['Object']
        self.x, self.y, self.z = self.sep.outputs['X'], self.sep.outputs['Y'], self.sep.outputs['Z']
        geo = m.node('ShaderNodeNewGeometry')
        ns = m.node('ShaderNodeSeparateXYZ')
        m.link(geo.outputs['Normal'], ns.inputs['Vector'])
        self.nx, self.ny, self.nz = ns.outputs['X'], ns.outputs['Y'], ns.outputs['Z']
        self.fin = attr(m, 'ns_fin')
        self.col = attr(m, 'ns_col', 'Color')
        self.dir = attr(m, 'ns_dir')
        self.var = attr(m, 'ns_var')


def attr(m, name, out='Fac'):
    n = m.node('ShaderNodeAttribute')
    n.attribute_type = 'GEOMETRY'
    n.attribute_name = name
    return n.outputs[out]


def mul(m, a, b):
    return m.math('MULTIPLY', a, b)


def add(m, *xs):
    out = xs[0]
    for x in xs[1:]:
        out = m.math('ADD', out, x)
    return out


def sub(m, a, b):
    return m.math('SUBTRACT', a, b)


def is_k(m, s, k):
    return m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', s, float(k))), 0.5)


def lighten(m, col, amount):
    return m.mix(amount, col, (1.0, 1.0, 1.0, 1.0))


def darken(m, col, f):
    return m.mix(1.0, col, (f, f, f, 1.0), blend='MULTIPLY')


def isolines(m, vector, scale, width, detail=1.0):
    """Thin wandering lines (scratches, creases): where a noise crosses its middle."""
    n = noise(m, vector, scale=scale, detail=detail)
    return ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', n, 0.5)), width, 0.0)


def along(c):
    """(a, b): coordinates along and across a part's grain axis (ns_dir: 0 X, 1 Y, 2 Z)."""
    m = c.m
    d1 = is_k(m, c.dir, 1)
    d2 = is_k(m, c.dir, 2)
    a = mix_float(m, d1, c.x, c.y)
    a = mix_float(m, d2, a, c.z)
    b = mix_float(m, d1, c.y, c.x)
    b = mix_float(m, d2, b, add(m, c.x, c.y))
    return a, b


def grain_vec(c, sa, sb):
    """Object coordinates stretched along the grain axis (sa) and across it (sb)."""
    m = c.m
    a, b = along(c)
    return vec(m, mul(m, a, sa), mul(m, b, sb), mul(m, add(m, c.z, c.x), sb))


def cell2(m, x, y, sx, sy, ox=0.0, oy=0.0):
    """(local u, local v, cell id hash 0..1) of a rectangular grid of cells sx x sy (meters)."""
    fx = m.math('DIVIDE', sub(m, x, ox), sx)
    fy = m.math('DIVIDE', sub(m, y, oy), sy)
    ix = m.math('FLOOR', fx)
    iy = m.math('FLOOR', fy)
    lu = sub(m, fx, ix)
    lv = sub(m, fy, iy)
    wn = m.node('ShaderNodeTexWhiteNoise')
    wn.noise_dimensions = '2D'
    m.link(vec(m, ix, iy, 0.0), wn.inputs['Vector'])
    return lu, lv, wn.outputs['Value']


def f_steel(c):
    m = c.m
    a, b = along(c)
    br = noise(m, vec(m, mul(m, a, 4.0), mul(m, b, 600.0), mul(m, c.z, 600.0)), detail=3.0)
    smudge = ramp(m, noise(m, c.P, scale=11.0, detail=4.0), 0.56, 0.8)
    prints = ramp(m, noise(m, c.P, scale=40.0, detail=3.0, distortion=1.2), 0.62, 0.72)
    col = m.mix(br, H(0x8f9396), H(0xb8bcc0))
    col = m.mix(mul(m, smudge, 0.14), col, H(0x74777a))
    rough = add(m, 0.24, mul(m, br, 0.05), mul(m, smudge, 0.07), mul(m, prints, 0.08))
    return col, rough, 1.0, mul(m, br, 0.002)


def f_chrome(c):
    m = c.m
    sm = ramp(m, noise(m, c.P, scale=12.0, detail=4.0), 0.45, 0.8)
    pit = ramp(m, noise(m, c.P, scale=500.0, detail=2.0), 0.74, 0.8)
    rust = mul(m, ramp(m, noise(m, c.P, scale=60.0, detail=3.0), 0.72, 0.8), c.var)
    col = m.mix(mul(m, pit, 0.6), H(0xd4d7da), H(0x77736c))
    col = m.mix(mul(m, rust, 0.8), col, H(0x6b4a2e))
    rough = add(m, 0.05, mul(m, sm, 0.10), mul(m, pit, 0.25), mul(m, rust, 0.5))
    return col, rough, mix_float(m, rust, 1.0, 0.3), mul(m, pit, -0.01)


def f_alu(c):
    m = c.m
    a, b = along(c)
    br = noise(m, vec(m, mul(m, a, 6.0), mul(m, b, 600.0), mul(m, c.z, 300.0)), detail=3.0)
    sm = ramp(m, noise(m, c.P, scale=10.0, detail=4.0), 0.5, 0.8)
    oxid = ramp(m, noise(m, c.P, scale=4.0, detail=4.0), 0.55, 0.8)
    col = m.mix(br, H(0xa2a6a9), H(0xc0c4c7))
    col = m.mix(mul(m, oxid, 0.4), col, H(0x8d8f8c))
    return m.mix(mul(m, sm, 0.3), col, H(0x7c7f82)), add(m, 0.32, mul(m, br, 0.06), mul(m, sm, 0.12), mul(m, oxid, 0.15)), 1.0, mul(m, br, 0.002)


def f_bronze(c):
    """Dark bronze anodized storefront aluminum (alu_bronze), chalky where the sun and rain have been at it."""
    m = c.m
    a, b = along(c)
    br = noise(m, vec(m, mul(m, a, 6.0), mul(m, b, 500.0), mul(m, c.z, 300.0)), detail=3.0)
    chalk = ramp(m, noise(m, c.P, scale=3.0, detail=4.0), 0.5, 0.85)
    scr = mul(m, isolines(m, scaled(m, c.sep, 30.0, 30.0, 4.0), 1.0, 0.008, 2.0), ramp(m, noise(m, c.P, scale=5.0, detail=2.0), 0.45, 0.65))
    col = m.mix(br, c.col, lighten(m, c.col, 0.08))
    col = m.mix(mul(m, chalk, 0.35), col, H(0x5d5852))
    col = m.mix(mul(m, scr, 0.6), col, H(0x9a9690))
    rough = add(m, 0.36, mul(m, br, 0.05), mul(m, chalk, 0.2), mul(m, scr, 0.1))
    return col, rough, mix_float(m, chalk, 0.9, 0.6), mul(m, br, 0.002)


def f_powder(c):
    """Powder-coated steel (gym frames, racks): orange-peel satin, chipped to grey steel on the edges, scuffed."""
    m = c.m
    peel = noise(m, c.P, scale=700.0, detail=1.0)
    edge = ramp(m, convex_edges(m, c.tc, distance=0.0015, samples=10, breakup_scale=90.0), 0.62, 0.78)
    scuff = mul(m, isolines(m, scaled(m, c.sep, 12.0, 12.0, 60.0), 1.0, 0.006, 2.0), ramp(m, noise(m, c.P, scale=4.0, detail=2.0), 0.5, 0.68))
    col = m.mix(mul(m, peel, 0.15), c.col, lighten(m, c.col, 0.06))
    col = m.mix(mul(m, scuff, 0.35), col, lighten(m, c.col, 0.18))
    col = m.mix(edge, col, H(0x8b8e93))
    rough = mix_float(m, edge, add(m, 0.42, mul(m, peel, 0.08), mul(m, scuff, 0.1)), 0.3)
    return col, rough, mix_float(m, edge, 0.0, 1.0), add(m, mul(m, peel, 0.004), mul(m, edge, -0.02))


def f_paint(c):
    """Paint on steel or wood (doors, frames, trim): roller stipple, chipped to primer and bare on edges, scuffed low."""
    m = c.m
    stip = noise(m, c.P, scale=350.0, detail=2.0)
    edge = convex_edges(m, c.tc, distance=0.003, samples=10, breakup_scale=50.0, gain=5.0)
    chip = ramp(m, edge, 0.55, 0.7)
    bare = ramp(m, edge, 0.76, 0.9)
    col = m.mix(mul(m, stip, 0.2), c.col, darken(m, c.col, 0.9))
    col = m.mix(chip, col, m.mix(0.55, H(0x8a8478), c.col))
    col = m.mix(bare, col, H(0x5f6265))
    scuff = mul(m, ramp(m, noise(m, scaled(m, c.sep, 4.0, 4.0, 60.0), detail=4.0), 0.56, 0.7), ramp(m, c.z, 0.6, 0.05))
    col = m.mix(mul(m, scuff, 0.7), col, H(0x2e2b27))
    scr = mul(m, isolines(m, scaled(m, c.sep, 8.0, 8.0, 260.0), 1.0, 0.007, 2.0), mul(m, ramp(m, noise(m, c.P, scale=5.0, detail=2.0), 0.45, 0.62), 0.8))
    col = m.mix(mul(m, scr, 0.55), col, H(0xa39d8f))
    rough = add(m, 0.44, mul(m, stip, 0.1), mul(m, chip, 0.15), mul(m, scuff, 0.15))
    return col, rough, mul(m, bare, 0.8), add(m, mul(m, stip, 0.008), mul(m, chip, -0.05), mul(m, scr, -0.015))


def f_wallpaint(c):
    """Interior latex on plaster or drywall: an orange-peel roller texture, patched and repainted (patches a shade off),
    scuffed black at shoe and cart height, rubbed grey at shoulder height, nail holes and a few hairline cracks."""
    m = c.m
    peel = noise(m, c.P, scale=260.0, detail=2.0)
    roll = noise(m, scaled(m, c.sep, 1.5, 1.5, 0.4), detail=3.0)
    patch = ramp(m, noise(m, c.P, scale=0.9, detail=3.0), 0.62, 0.66)
    col = m.mix(mul(m, roll, 0.16), c.col, darken(m, c.col, 0.9))
    col = m.mix(mul(m, patch, 0.35), col, lighten(m, c.col, 0.12))
    scuff = mul(m, mul(m, ramp(m, noise(m, scaled(m, c.sep, 3.0, 3.0, 40.0), detail=4.0), 0.6, 0.72), ramp(m, c.z, 0.55, 0.05)),
                ramp(m, noise(m, c.P, scale=1.4, detail=2.0), 0.35, 0.6))
    col = m.mix(mul(m, scuff, 0.8), col, H(0x2a2725))
    rub = mul(m, mul(m, ramp(m, c.z, 1.0, 1.25), ramp(m, c.z, 1.6, 1.35)), ramp(m, noise(m, c.P, scale=2.0, detail=3.0), 0.45, 0.75))
    col = m.mix(mul(m, rub, 0.22), col, H(0x6c6458))
    holes = ramp(m, noise(m, c.P, scale=90.0, detail=1.0), 0.79, 0.81)
    col = m.mix(mul(m, holes, 0.7), col, H(0x3a3530))
    crack = mul(m, isolines(m, scaled(m, c.sep, 2.0, 2.0, 2.0), 1.0, 0.004, 4.0), ramp(m, noise(m, c.P, scale=0.7, detail=2.0), 0.6, 0.7))
    col = m.mix(mul(m, crack, 0.5), col, H(0x5a554c))
    rough = add(m, 0.72, mul(m, peel, 0.08), mul(m, scuff, -0.1), mul(m, rub, -0.15))
    return col, rough, 0.0, add(m, mul(m, peel, 0.004), mul(m, holes, -0.06), mul(m, crack, -0.02), mul(m, patch, 0.002))


def f_brickpaint(c):
    """Painted common brick (the gym's warehouse walls): coursing in object space on the wall plane (ns_dir 0: a wall
    in XZ, 1: in YZ), mortar joints recessed under the paint, paint worn off to red brick low down and at corners."""
    m = c.m
    a = mix_float(m, is_k(m, c.dir, 1), c.x, c.y)
    course = 0.0675
    brick = 0.2032
    row = m.math('FLOOR', m.math('DIVIDE', c.z, course))
    off = mul(m, m.math('FRACT', mul(m, row, 0.5)), brick)
    u = m.math('DIVIDE', add(m, a, off), brick)
    fu = sub(m, u, m.math('FLOOR', u))
    fv = m.math('FRACT', m.math('DIVIDE', c.z, course))
    ju = ramp(m, m.math('ABSOLUTE', sub(m, fu, 0.5)), 0.455, 0.475)
    jv = ramp(m, m.math('ABSOLUTE', sub(m, fv, 0.5)), 0.36, 0.43)
    joint = m.math('MAXIMUM', ju, jv)
    face_n = noise(m, c.P, scale=30.0, detail=3.0)
    wn = m.node('ShaderNodeTexWhiteNoise')
    wn.noise_dimensions = '2D'
    m.link(vec(m, m.math('FLOOR', u), row, 0.0), wn.inputs['Vector'])
    tone = wn.outputs['Value']
    loss = mul(m, ramp(m, noise(m, c.P, scale=2.6, detail=5.0), 0.66, 0.71), ramp(m, c.z, 0.9, 0.05))
    loss = m.math('MAXIMUM', loss, mul(m, ramp(m, noise(m, c.P, scale=9.0, detail=5.0), 0.74, 0.77), 0.7))
    brick_col = m.mix(tone, H(0x7a3a2a), H(0x8f4a35))
    brick_col = m.mix(joint, brick_col, H(0x8d877c))
    paint = m.mix(mul(m, tone, 0.12), c.col, darken(m, c.col, 0.92))
    paint = m.mix(mul(m, joint, 0.3), paint, darken(m, c.col, 0.75))
    col = m.mix(loss, paint, brick_col)
    drip = mul(m, ramp(m, noise(m, scaled(m, c.sep, 40.0, 40.0, 1.0), detail=3.0), 0.6, 0.75), 0.25)
    col = m.mix(drip, col, darken(m, c.col, 0.8))
    rough = add(m, mix_float(m, loss, 0.62, 0.86), mul(m, joint, 0.1), mul(m, face_n, 0.06))
    h = add(m, mul(m, joint, -0.6), mul(m, face_n, 0.15), mul(m, loss, -0.05))
    return col, rough, 0.0, h


def f_wood(c):
    """Varnished wood (oak by default; ns_col tints the stain): grain along ns_dir, pores, worn through the finish at
    the edges and where hands go."""
    m = c.m
    v = grain_vec(c, 1.2, 60.0)
    wave = m.node('ShaderNodeTexWave', Scale=1.0, Distortion=9.0, **{'Detail': 6.0})
    wave.wave_type = 'BANDS'
    wave.bands_direction = 'X'
    m.link(v, wave.inputs['Vector'])
    g = wave.outputs['Fac']
    pores = ramp(m, noise(m, grain_vec(c, 30.0, 900.0), detail=2.0), 0.62, 0.7)
    col = m.mix(mul(m, g, 0.55), c.col, darken(m, c.col, 0.62))
    col = m.mix(mul(m, pores, 0.5), col, darken(m, c.col, 0.45))
    edge = ramp(m, convex_edges(m, c.tc, distance=0.003, samples=10, breakup_scale=40.0), 0.5, 0.7)
    col = m.mix(mul(m, edge, 0.5), col, lighten(m, c.col, 0.2))
    # fine scuffs through the varnish, mostly along the run (hands and bags slide along a counter), and few
    scr = mul(m, isolines(m, grain_vec(c, 14.0, 70.0), 1.0, 0.0035, 2.0), ramp(m, noise(m, c.P, scale=3.0, detail=2.0), 0.55, 0.7))
    col = m.mix(mul(m, scr, 0.3), col, lighten(m, c.col, 0.14))
    rough = add(m, 0.36, mul(m, g, 0.08), mul(m, pores, 0.12), mul(m, edge, 0.2), mul(m, scr, 0.12))
    return col, rough, 0.0, add(m, mul(m, g, 0.01), mul(m, pores, -0.01), mul(m, scr, -0.01))


def f_laminate(c):
    """High-pressure laminate: a woodgrain print (ns_var < 0.5) or a speckle (ns_var >= 0.5); worn lighter where hands
    slide, chipped at the edge to the board, stained."""
    m = c.m
    v = grain_vec(c, 2.0, 40.0)
    wave = m.node('ShaderNodeTexWave', Scale=1.0, Distortion=6.0, **{'Detail': 3.0})
    wave.wave_type = 'BANDS'
    m.link(v, wave.inputs['Vector'])
    gr = wave.outputs['Fac']
    sp1 = ramp(m, noise(m, c.P, scale=330.0, detail=1.0), 0.68, 0.71)
    sp2 = ramp(m, noise(m, vec(m, c.x, c.y, add(m, c.z, 3.1)), scale=420.0, detail=1.0), 0.3, 0.27)
    speck = ramp(m, c.var, 0.45, 0.55)
    col_g = m.mix(mul(m, gr, 0.45), c.col, darken(m, c.col, 0.7))
    col_s = m.mix(mul(m, sp1, 0.7), c.col, darken(m, c.col, 0.55))
    col_s = m.mix(mul(m, sp2, 0.5), col_s, lighten(m, c.col, 0.5))
    col = m.mix(speck, col_g, col_s)
    worn = ramp(m, noise(m, c.P, scale=1.6, detail=3.0), 0.45, 0.7)
    col = m.mix(mul(m, worn, 0.14), col, lighten(m, c.col, 0.3))
    stain = ramp(m, noise(m, c.P, scale=5.0, detail=5.0, distortion=0.6), 0.7, 0.76)
    col = m.mix(mul(m, stain, 0.4), col, H(0x6a5a40))
    scr = mul(m, isolines(m, scaled(m, c.sep, 120.0, 9.0, 1.0), 1.0, 0.005, 2.0), ramp(m, noise(m, c.P, scale=4.0, detail=2.0), 0.4, 0.6))
    col = m.mix(mul(m, scr, 0.35), col, lighten(m, c.col, 0.35))
    chip = mul(m, ramp(m, convex_edges(m, c.tc, distance=0.0015, samples=10, breakup_scale=40.0), 0.7, 0.82),
               ramp(m, noise(m, c.P, scale=6.0, detail=2.0), 0.5, 0.62))
    col = m.mix(mul(m, chip, 0.85), col, H(0x8f7350))
    rough = add(m, 0.36, mul(m, worn, -0.1), mul(m, stain, 0.12), mul(m, scr, 0.08), mul(m, chip, 0.4))
    return col, rough, 0.0, add(m, mul(m, sp1, 0.002), mul(m, chip, -0.05), mul(m, scr, -0.004))


def f_particle(c):
    m = c.m
    n = noise(m, c.P, scale=400.0, detail=3.0)
    fl = ramp(m, noise(m, c.P, scale=160.0, detail=2.0), 0.6, 0.66)
    col = m.mix(n, H(0x9c7a50), H(0x6e5232))
    col = m.mix(mul(m, fl, 0.6), col, H(0xc2a272))
    return col, 0.9, 0.0, add(m, mul(m, n, 0.1), mul(m, fl, 0.05))


def f_plywood(c):
    m = c.m
    v = grain_vec(c, 1.0, 30.0)
    wave = m.node('ShaderNodeTexWave', Scale=1.0, Distortion=14.0, **{'Detail': 4.0})
    wave.wave_type = 'RINGS'
    m.link(v, wave.inputs['Vector'])
    g = wave.outputs['Fac']
    col = m.mix(mul(m, g, 0.5), c.col, darken(m, c.col, 0.75))
    knot = ramp(m, noise(m, c.P, scale=6.0, detail=2.0), 0.74, 0.78)
    col = m.mix(mul(m, knot, 0.7), col, darken(m, c.col, 0.5))
    return col, add(m, 0.7, mul(m, g, 0.1)), 0.0, mul(m, g, 0.01)


def f_carpet(c):
    """Low-pile commercial carpet (display platforms, mats): loops, flattened and darker where it's walked."""
    m = c.m
    loops = noise(m, c.P, scale=1400.0, detail=2.0)
    mott = noise(m, c.P, scale=8.0, detail=3.0)
    worn = ramp(m, noise(m, c.P, scale=1.5, detail=3.0), 0.5, 0.8)
    col = m.mix(mul(m, loops, 0.5), c.col, darken(m, c.col, 0.7))
    col = m.mix(mul(m, mott, 0.2), col, lighten(m, c.col, 0.1))
    col = m.mix(mul(m, worn, 0.4), col, H(0x4a453e))
    return col, 0.95, 0.0, add(m, mul(m, loops, 0.3), mul(m, worn, -0.1))


def f_cloth(c):
    m = c.m
    period = 0.0018
    wa = m.math('SINE', mul(m, add(m, c.x, c.z), 2 * math.pi / period))
    wb = m.math('SINE', mul(m, add(m, c.y, c.z), 2 * math.pi / period))
    weave = mul(m, add(m, mul(m, wa, wb), 1.0), 0.5)
    heather = noise(m, c.P, scale=500.0, detail=2.0)
    fade = ramp(m, noise(m, c.P, scale=4.0, detail=3.0), 0.35, 0.75)
    pill = ramp(m, noise(m, c.P, scale=900.0, detail=1.0), 0.72, 0.76)
    col = m.mix(mul(m, heather, 0.3), c.col, lighten(m, c.col, 0.08))
    col = m.mix(mul(m, fade, 0.12), col, lighten(m, c.col, 0.12))
    col = m.mix(mul(m, pill, 0.25), col, lighten(m, c.col, 0.15))
    return col, 0.93, 0.0, add(m, mul(m, weave, 0.05), mul(m, heather, 0.02))


def f_canvas(c):
    """Heavy canvas or duck (the heavy bag's cover, a curtain): a coarse basket weave."""
    m = c.m
    period = 0.003
    wa = m.math('SINE', mul(m, add(m, c.x, c.z), 2 * math.pi / period))
    wb = m.math('SINE', mul(m, add(m, c.y, mul(m, c.z, -1.0)), 2 * math.pi / period))
    weave = mul(m, add(m, mul(m, wa, wb), 1.0), 0.5)
    mott = noise(m, c.P, scale=6.0, detail=4.0)
    col = m.mix(mul(m, mott, 0.25), c.col, darken(m, c.col, 0.82))
    col = m.mix(mul(m, weave, 0.2), col, lighten(m, c.col, 0.1))
    return col, 0.9, 0.0, add(m, mul(m, weave, 0.12), mul(m, mott, 0.05))


def f_vinyl(c):
    """Upholstery vinyl (a bench pad, a stool): a fine leather-look grain, shiny where sat on, cracked at the seams,
    a tear patched with tape here and there."""
    m = c.m
    vor = m.node('ShaderNodeTexVoronoi', Scale=900.0)
    m.link(c.P, vor.inputs['Vector'])
    grain = vor.outputs['Distance']
    crack = mul(m, isolines(m, scaled(m, c.sep, 30.0, 30.0, 30.0), 1.0, 0.01, 3.0),
                ramp(m, convex_edges(m, c.tc, distance=0.006, samples=8, breakup_scale=30.0), 0.2, 0.5))
    col = m.mix(mul(m, grain, 0.3), c.col, darken(m, c.col, 0.85))
    col = m.mix(mul(m, crack, 0.35), col, lighten(m, c.col, 0.25))
    rough = add(m, 0.5, mul(m, grain, 0.1), mul(m, crack, 0.2))
    return col, rough, 0.0, add(m, mul(m, grain, 0.08), mul(m, crack, -0.1))


def f_leather(c):
    """Heavy-bag leather: pebbled, creased round the middle where it's hit, scuffed pale."""
    m = c.m
    vor = m.node('ShaderNodeTexVoronoi', Scale=500.0)
    m.link(c.P, vor.inputs['Vector'])
    peb = vor.outputs['Distance']
    crease = isolines(m, scaled(m, c.sep, 14.0, 14.0, 30.0), 1.0, 0.02, 3.0)
    scuff = ramp(m, noise(m, c.P, scale=9.0, detail=4.0), 0.62, 0.8)
    col = m.mix(mul(m, peb, 0.25), c.col, darken(m, c.col, 0.8))
    col = m.mix(mul(m, crease, 0.5), col, darken(m, c.col, 0.6))
    col = m.mix(mul(m, scuff, 0.18), col, lighten(m, c.col, 0.14))
    rough = add(m, 0.48, mul(m, peb, 0.1), mul(m, scuff, 0.2))
    return col, rough, 0.0, add(m, mul(m, peb, 0.1), mul(m, crease, -0.2))


def f_rubber(c):
    m = c.m
    g = noise(m, c.P, scale=900.0, detail=1.0)
    dust = ramp(m, noise(m, c.P, scale=20.0, detail=3.0), 0.5, 0.8)
    scuff = mul(m, isolines(m, scaled(m, c.sep, 20.0, 20.0, 20.0), 1.0, 0.008, 2.0), 0.4)
    col = m.mix(mul(m, dust, 0.12), c.col, H(0x5a5650))
    col = m.mix(mul(m, scuff, 0.3), col, lighten(m, c.col, 0.1))
    return col, add(m, 0.78, mul(m, g, 0.08), mul(m, scuff, -0.1)), 0.0, mul(m, g, 0.01)


def f_plastic(c):
    m = c.m
    g = noise(m, c.P, scale=1800.0, detail=1.0)
    edges = mul(m, convex_edges(m, c.tc, distance=0.0012, samples=8, breakup_scale=220.0), 0.6)
    yell = ramp(m, noise(m, c.P, scale=3.0, detail=3.0), 0.4, 0.8)
    col = m.mix(mul(m, edges, 0.5), c.col, lighten(m, c.col, 0.25))
    col = m.mix(mul(m, yell, mul(m, c.var, 0.25)), col, H(0xb8a878))
    rough = sub(m, add(m, 0.48, mul(m, g, 0.06)), mul(m, edges, 0.1))
    return col, rough, 0.0, mul(m, g, 0.004)


def f_gloss(c):
    m = c.m
    scratch = ramp(m, noise(m, c.P, scale=900.0, detail=4.0, distortion=2.0), 0.62, 0.7)
    smear = ramp(m, noise(m, c.P, scale=14.0, detail=3.0), 0.5, 0.8)
    return c.col, add(m, 0.1, mul(m, scratch, 0.12), mul(m, smear, 0.12)), 0.0, 0.0


def f_paper(c):
    m = c.m
    fib = noise(m, scaled(m, c.sep, 900.0, 300.0, 900.0), detail=2.0)
    col = m.mix(mul(m, fib, 0.15), c.col, darken(m, c.col, 0.85))
    return col, add(m, 0.8, mul(m, fib, 0.1)), 0.0, mul(m, fib, 0.004)


def f_cardboard(c):
    m = c.m
    flute = m.math('SINE', mul(m, add(m, c.x, c.y), 2 * math.pi / 0.008))
    fib = noise(m, c.P, scale=300.0, detail=3.0)
    stain = ramp(m, noise(m, c.P, scale=5.0, detail=4.0), 0.6, 0.75)
    col = m.mix(mul(m, fib, 0.25), c.col, darken(m, c.col, 0.85))
    col = m.mix(mul(m, stain, 0.35), col, darken(m, c.col, 0.65))
    return col, 0.88, 0.0, add(m, mul(m, flute, 0.01), mul(m, fib, 0.02))


def f_cork(c):
    m = c.m
    vor = m.node('ShaderNodeTexVoronoi', Scale=900.0)
    m.link(c.P, vor.inputs['Vector'])
    gran = vor.outputs['Distance']
    tone = noise(m, c.P, scale=300.0, detail=2.0)
    col = m.mix(tone, H(0xa77c48), H(0xc9995c))
    col = m.mix(mul(m, ramp(m, gran, 0.15, 0.0), 0.8), col, H(0x5a3a1c))
    pins = ramp(m, noise(m, c.P, scale=180.0, detail=2.0), 0.74, 0.8)
    col = m.mix(mul(m, pins, 0.7), col, H(0x3a2614))
    return col, 0.9, 0.0, add(m, mul(m, gran, 0.4), mul(m, tone, 0.1), mul(m, pins, -0.3))


def f_blackglass(c):
    """Structural black glass (the old storefront's bulkheads): a mirror-black gloss, hazed, crazed and cracked."""
    m = c.m
    haze = ramp(m, noise(m, c.P, scale=4.0, detail=4.0), 0.4, 0.8)
    crack = mul(m, isolines(m, scaled(m, c.sep, 3.0, 3.0, 3.0), 1.0, 0.005, 5.0), ramp(m, noise(m, c.P, scale=1.2, detail=2.0), 0.58, 0.64))
    col = m.mix(mul(m, haze, 0.3), c.col, H(0x302c2a))
    col = m.mix(crack, col, H(0x8a8580))
    rough = add(m, 0.05, mul(m, haze, 0.2), mul(m, crack, 0.4))
    return col, rough, 0.0, mul(m, crack, -0.04)


def f_enamel(c):
    """Baked enamel on a cabinet (the cooler): orange peel, wiped in arcs, scratched, scuffed low."""
    m = c.m
    peel = noise(m, c.P, scale=500.0, detail=1.0)
    wipe = ramp(m, noise(m, scaled(m, c.sep, 50.0, 50.0, 3.0), detail=3.0), 0.56, 0.7)
    col = m.mix(mul(m, wipe, 0.08), c.col, lighten(m, c.col, 0.2))
    scr = mul(m, isolines(m, scaled(m, c.sep, 6.0, 6.0, 140.0), 1.0, 0.006, 2.0), ramp(m, noise(m, c.P, scale=6.0, detail=2.0), 0.45, 0.65))
    col = m.mix(mul(m, scr, 0.3), col, lighten(m, c.col, 0.35))
    chips = ramp(m, convex_edges(m, c.tc, distance=0.002, samples=8, breakup_scale=60.0), 0.45, 0.6)
    col = m.mix(chips, col, H(0x6f7275))
    rough = mix_float(m, chips, add(m, 0.28, mul(m, peel, 0.06), mul(m, wipe, -0.06), mul(m, scr, 0.1)), 0.5)
    return col, rough, mix_float(m, chips, 0.0, 0.6), add(m, mul(m, peel, 0.008), mul(m, chips, -0.04))


def f_velvet(c):
    m = c.m
    n = noise(m, c.P, scale=600.0, detail=3.0)
    crush = ramp(m, noise(m, c.P, scale=12.0, detail=2.0), 0.45, 0.65)
    col = m.mix(mul(m, crush, 0.12), c.col, lighten(m, c.col, 0.12))
    col = m.mix(mul(m, n, 0.12), col, darken(m, c.col, 0.85))
    return col, 0.92, 0.0, mul(m, n, 0.05)


def f_brass(c):
    m = c.m
    tarn = mul(m, ramp(m, noise(m, c.P, scale=5.0, detail=3.0), 0.45, 0.85), 0.6)
    rub = ramp(m, convex_edges(m, c.tc, distance=0.003, samples=8, breakup_scale=30.0), 0.3, 0.6)
    col = m.mix(tarn, H(0xb48c4a), H(0x7a6034))
    col = m.mix(rub, col, H(0xd9b46a))
    return col, add(m, 0.25, mul(m, tarn, 0.3), mul(m, rub, -0.12)), 1.0, mul(m, tarn, 0.002)


def f_foam(c):
    """EVA foam (gym mats, a roller): fine closed cells, pressed shiny in patches."""
    m = c.m
    vor = m.node('ShaderNodeTexVoronoi', Scale=1600.0)
    m.link(c.P, vor.inputs['Vector'])
    cells = vor.outputs['Distance']
    pressed = ramp(m, noise(m, c.P, scale=3.0, detail=3.0), 0.5, 0.75)
    col = m.mix(mul(m, cells, 0.2), c.col, darken(m, c.col, 0.85))
    col = m.mix(mul(m, pressed, 0.25), col, darken(m, c.col, 0.8))
    return col, add(m, 0.82, mul(m, pressed, -0.2)), 0.0, mul(m, cells, 0.05)


def f_ceiltile(c):
    """Mineral-fiber acoustic tile (2 x 4 ft lay-in): fissured, a hair yellowed, water-stained in rings here and there
    (ns_var picks which tiles), a replaced tile whiter than its neighbors."""
    m = c.m
    vor = m.node('ShaderNodeTexVoronoi', Scale=120.0)
    vor.feature = 'DISTANCE_TO_EDGE'
    m.link(add_vec(m, c.P, noise_vec(m, c.P, 40.0), 0.004), vor.inputs['Vector'])
    fiss = ramp(m, vor.outputs['Distance'], 0.05, 0.0)
    pin = ramp(m, noise(m, c.P, scale=700.0, detail=1.0), 0.66, 0.7)
    stain_mask = ramp(m, c.var, 0.935, 0.945)
    sd = noise(m, c.P, scale=2.4, detail=4.0)
    stain = mul(m, mul(m, stain_mask, ramp(m, sd, 0.5, 0.64)), 0.6)
    ring = mul(m, mul(m, stain_mask, ramp(m, m.math('ABSOLUTE', sub(m, sd, 0.52)), 0.01, 0.0)), 0.7)
    newer = ramp(m, c.var, 0.06, 0.04)
    col = m.mix(mul(m, fiss, 0.55), c.col, darken(m, c.col, 0.6))
    col = m.mix(mul(m, pin, 0.4), col, darken(m, c.col, 0.7))
    col = m.mix(mul(m, stain, 0.35), col, H(0xa08a5c))
    col = m.mix(mul(m, ring, 0.7), col, H(0x7a6440))
    col = m.mix(newer, col, lighten(m, c.col, 0.18))
    return col, 0.94, 0.0, add(m, mul(m, fiss, -0.6), mul(m, pin, -0.2))


def f_concrete(c):
    m = c.m
    blot = noise(m, c.P, scale=4.0, detail=5.0)
    pits = ramp(m, noise(m, c.P, scale=220.0, detail=1.0), 0.68, 0.72)
    col = m.mix(mul(m, blot, 0.5), c.col, darken(m, c.col, 0.75))
    col = m.mix(mul(m, pits, 0.5), col, H(0x3a3936))
    return col, 0.88, 0.0, sub(m, mul(m, blot, 0.3), pits)


def f_slatwall(c):
    """Slatwall panel (MDF faced in melamine): horizontal grooves every 3 in (the parts' own geometry), the face a
    satin melamine tinted by ns_col, chipped at the groove lips."""
    m = c.m
    sp = noise(m, c.P, scale=400.0, detail=1.0)
    chip = ramp(m, convex_edges(m, c.tc, distance=0.0015, samples=8, breakup_scale=120.0), 0.5, 0.7)
    col = m.mix(mul(m, sp, 0.1), c.col, darken(m, c.col, 0.9))
    col = m.mix(chip, col, H(0x9c7d58))
    return col, add(m, 0.42, mul(m, chip, 0.4)), 0.0, mul(m, chip, -0.04)


def f_pegboard(c):
    """Hardboard pegboard: 1 in grid of quarter-inch holes (shader holes, dark), a tempered sheen, painted edges."""
    m = c.m
    lu, lv, h = cell2(m, mix_float(m, is_k(m, c.dir, 1), c.x, c.y), c.z, 0.0254, 0.0254)
    d = m.math('SQRT', add(m, mul(m, sub(m, lu, 0.5), sub(m, lu, 0.5)), mul(m, sub(m, lv, 0.5), sub(m, lv, 0.5))))
    hole = smooth_less(m, mul(m, d, 0.0254), 0.0032, 0.0004)
    fib = noise(m, c.P, scale=200.0, detail=3.0)
    col = m.mix(mul(m, fib, 0.2), c.col, darken(m, c.col, 0.85))
    col = m.mix(hole, col, H(0x0c0a08))
    return col, add(m, 0.6, mul(m, hole, 0.3)), 0.0, mul(m, hole, -0.4)


def f_galv(c):
    m = c.m
    spangle = noise(m, c.P, scale=60.0, detail=1.0, distortion=1.5)
    patina = noise(m, c.P, scale=5.0, detail=4.0)
    col = m.mix(mul(m, spangle, 0.25), H(0x9ea3a6), H(0xc3c7c9))
    col = m.mix(mul(m, patina, 0.5), col, H(0x7d8184))
    return col, add(m, 0.45, mul(m, sub(m, spangle, 0.5), 0.15)), 0.9, mul(m, spangle, 0.002)


def f_chalk(c):
    m = c.m
    n = noise(m, c.P, scale=80.0, detail=4.0)
    col = m.mix(mul(m, n, 0.3), H(0xf0eee8), H(0xc9c6bd))
    return col, 0.97, 0.0, mul(m, n, 0.2)


def f_glassbaked(c):
    """Glass that bakes (a lens, a lamp's chimney, the tablet's bezel): dark, glossy."""
    m = c.m
    film = ramp(m, noise(m, c.P, scale=8.0, detail=4.0), 0.5, 0.8)
    return m.mix(0.5, c.col, H(0x0d1014)), add(m, 0.04, mul(m, film, 0.1)), 0.0, 0.0


def f_prism(c):
    """Prism-glass transom tiles (4 in squares with ribs), violet with age: lit from behind they glow a little."""
    m = c.m
    a = mix_float(m, is_k(m, c.dir, 1), c.x, c.y)
    ox, oz, size = c.kit.params.get('prism', (0.0, 0.0, 0.1016))
    lu, lv, h = cell2(m, a, c.z, size, size, ox, oz)
    rib = m.math('SINE', mul(m, lv, 2 * math.pi * 7.0))
    frame = mul(m, m.math('MAXIMUM', ramp(m, m.math('ABSOLUTE', sub(m, lu, 0.5)), 0.46, 0.5), ramp(m, m.math('ABSOLUTE', sub(m, lv, 0.5)), 0.46, 0.5)), 0.4)
    col = m.mix(mul(m, h, 0.4), c.col, H(0x8a7f9a))
    col = m.mix(mul(m, add(m, mul(m, rib, 0.5), 0.5), 0.25), col, lighten(m, c.col, 0.3))
    col = m.mix(frame, col, H(0x2a2622))
    return col, add(m, 0.12, mul(m, frame, 0.5)), mul(m, frame, 0.6), add(m, mul(m, rib, 0.01), mul(m, frame, -0.05))


def f_felt(c):
    m = c.m
    n = noise(m, c.P, scale=500.0, detail=4.0)
    return m.mix(mul(m, n, 0.5), c.col, lighten(m, c.col, 0.15)), 0.95, 0.0, mul(m, n, 0.05)


def f_lino(c):
    """Sheet linoleum or rubber stair tread (a counter's kick, a step): marbled, worn."""
    m = c.m
    marb = noise(m, c.P, scale=12.0, detail=6.0, distortion=2.0)
    col = m.mix(mul(m, marb, 0.4), c.col, lighten(m, c.col, 0.2))
    return col, add(m, 0.45, mul(m, marb, 0.1)), 0.0, mul(m, marb, 0.004)


def f_knurl(c):
    """A barbell's or dumbbell's knurled steel (the grip runs along ns_dir): a diamond cross-hatch, chalk in it, rust
    blooming where hands sweat."""
    m = c.m
    a, b = along(c)
    ang = m.math('ARCTAN2', c.z, mix_float(m, is_k(m, c.dir, 1), c.y, c.x))
    p = 0.0016
    d1 = m.math('SINE', mul(m, add(m, a, mul(m, ang, 0.016)), 2 * math.pi / p))
    d2 = m.math('SINE', mul(m, sub(m, a, mul(m, ang, 0.016)), 2 * math.pi / p))
    kn = ramp(m, mul(m, d1, d2), 0.1, 0.6)
    chalk = mul(m, ramp(m, noise(m, c.P, scale=40.0, detail=3.0), 0.45, 0.7), 0.6)
    rust = ramp(m, noise(m, c.P, scale=25.0, detail=4.0), 0.68, 0.76)
    col = m.mix(kn, H(0x8d9195), H(0xb9bdc1))
    col = m.mix(mul(m, chalk, sub(m, 1.0, kn)), col, H(0xe8e6e0))
    col = m.mix(mul(m, rust, 0.6), col, H(0x6a4a30))
    rough = add(m, 0.32, mul(m, kn, -0.1), mul(m, chalk, 0.4), mul(m, rust, 0.3))
    return col, rough, mix_float(m, add(m, mul(m, chalk, 0.8), mul(m, rust, 0.5)), 1.0, 0.2), mul(m, kn, 0.06)


def f_corrugated(c):
    """Corrugated steel roof deck seen from below, painted (ns_col): the ribs are geometry; paint overspray, rust at the
    laps, a water stain or two."""
    m = c.m
    mott = noise(m, c.P, scale=2.0, detail=4.0)
    rust = ramp(m, noise(m, c.P, scale=6.0, detail=5.0), 0.68, 0.74)
    col = m.mix(mul(m, mott, 0.25), c.col, lighten(m, c.col, 0.1))
    col = m.mix(mul(m, rust, 0.6), col, H(0x5a3a22))
    return col, add(m, 0.6, mul(m, mott, 0.1), mul(m, rust, 0.2)), mul(m, rust, 0.2), mul(m, mott, 0.002)


def f_vct(c):
    """Vinyl composition tile, 12 in, laid by the five-and-dime and patched by St. Agnes (kit.params['vct']):
    a field of speckled putty tiles (a tone per tile), a darker border course, a few replacement tiles a shade off,
    cracked tiles, black heel marks; the traffic paths (door -> register -> racks -> booth) worn matte and pale,
    the old wax yellowed and dirty in the corners and along the walls."""
    m = c.m
    p = c.kit.params.get('vct', {})
    size = p.get('tile', 0.3048)
    ox, oy = p.get('origin', (0.0, 0.0))
    lu, lv, h = cell2(m, c.x, c.y, size, size, ox, oy)
    joint = m.math('MAXIMUM', ramp(m, m.math('ABSOLUTE', sub(m, lu, 0.5)), 0.493, 0.4985), ramp(m, m.math('ABSOLUTE', sub(m, lv, 0.5)), 0.493, 0.4985))
    chips = ramp(m, noise(m, c.P, scale=260.0, detail=1.0), 0.66, 0.70)
    chips2 = ramp(m, noise(m, vec(m, c.y, c.x, 4.0), scale=330.0, detail=1.0), 0.33, 0.29)
    swirl = noise(m, c.P, scale=14.0, detail=3.0, distortion=1.0)
    base = m.mix(mul(m, h, 0.5), c.col, darken(m, c.col, 0.88))
    replaced = ramp(m, h, 0.955, 0.96)
    base = m.mix(replaced, base, H(p.get('patch', 0xa29a86)))
    col = m.mix(mul(m, chips, 0.55), base, darken(m, c.col, 0.55))
    col = m.mix(mul(m, chips2, 0.45), col, lighten(m, c.col, 0.45))
    col = m.mix(mul(m, swirl, 0.12), col, lighten(m, c.col, 0.1))
    # border course along the walls
    bx0, bx1, by0, by1 = p.get('room', (-1e3, 1e3, -1e3, 1e3))
    bw = p.get('border', 0.0)
    if bw > 0:
        inb = m.math('MAXIMUM', m.math('MAXIMUM', ramp(m, c.x, bx0 + bw + 0.002, bx0 + bw), ramp(m, c.x, bx1 - bw - 0.002, bx1 - bw)),
                     m.math('MAXIMUM', ramp(m, c.y, by0 + bw + 0.002, by0 + bw), ramp(m, c.y, by1 - bw - 0.002, by1 - bw)))
        col = m.mix(inb, col, m.mix(mul(m, chips, 0.5), H(p.get('border_col', 0x3b3a34)), H(0x6b665a)))
    else:
        inb = 0.0
    # traffic paths
    wear = None
    for (pts, width) in p.get('paths', []):
        for (ax, ay), (bxp, byp) in zip(pts, pts[1:]):
            d = seg_dist(m, c.x, c.y, ax, ay, bxp, byp)
            w_ = smooth_less(m, d, width * 0.5, width * 0.5)
            wear = w_ if wear is None else m.math('MAXIMUM', wear, w_)
    if wear is None:
        wear = 0.0
    else:
        wear = mul(m, wear, ramp(m, noise(m, c.P, scale=2.5, detail=4.0), 0.15, 0.6))
    edges = mul(m, ramp(m, noise(m, c.P, scale=0.8, detail=3.0), 0.5, 0.8), 1.0)
    wall = c.kit.params.get('vct_walls', None)
    near_wall = 0.0
    if wall:
        x0, x1, y0, y1 = wall
        near_wall = m.math('MAXIMUM', m.math('MAXIMUM', ramp(m, c.x, x0 + 0.25, x0), ramp(m, c.x, x1 - 0.25, x1)),
                           m.math('MAXIMUM', ramp(m, c.y, y0 + 0.25, y0), ramp(m, c.y, y1 - 0.25, y1)))
    wax = m.math('MAXIMUM', mul(m, near_wall, 0.8), mul(m, edges, 0.25))
    col = m.mix(mul(m, wear, 0.45), col, H(p.get('worn', 0xb3ab98)))
    col = m.mix(mul(m, wax, 0.5), col, H(0x5c5340))
    heel = mul(m, mul(m, isolines(m, scaled(m, c.sep, 30.0, 30.0, 1.0), 1.0, 0.006, 2.0), ramp(m, noise(m, c.P, scale=3.0, detail=2.0), 0.55, 0.7)),
               add(m, 0.3, wear))
    col = m.mix(mul(m, heel, 0.85), col, H(0x161412))
    crack = mul(m, isolines(m, scaled(m, c.sep, 6.0, 6.0, 6.0), 1.0, 0.004, 4.0), ramp(m, h, 0.9, 0.92))
    col = m.mix(mul(m, crack, 0.8), col, H(0x2e2a24))
    col = m.mix(mul(m, joint, 0.6), col, H(0x3a362e))
    rough = add(m, 0.32, mul(m, wear, 0.28), mul(m, joint, 0.3), mul(m, wax, -0.08), mul(m, heel, 0.1))
    return col, rough, 0.0, add(m, mul(m, joint, -0.25), mul(m, crack, -0.3), mul(m, chips, 0.003))


def seg_dist(m, x, y, ax, ay, bx_, by_):
    """Distance from (x, y) to the segment a-b (constants)."""
    dx, dy = bx_ - ax, by_ - ay
    L2 = max(dx * dx + dy * dy, 1e-9)
    px = sub(m, x, ax)
    py = sub(m, y, ay)
    t = m.math('MAXIMUM', m.math('MINIMUM', m.math('DIVIDE', add(m, mul(m, px, dx), mul(m, py, dy)), L2), 1.0), 0.0)
    ex = sub(m, px, mul(m, t, dx))
    ey = sub(m, py, mul(m, t, dy))
    return m.math('SQRT', add(m, mul(m, ex, ex), mul(m, ey, ey)))


def f_rubberfloor(c):
    """Recycled rubber gym tile (rubber_gym, 1 m interlocking): black with color flecks, seams, chalk and sweat marks,
    worn grey under the machines' feet and on the paths, an inlaid logo (kit.params['logo']: image, rect)."""
    m = c.m
    p = c.kit.params.get('rubberfloor', {})
    size = p.get('tile', 1.0)
    ox, oy = p.get('origin', (0.0, 0.0))
    lu, lv, h = cell2(m, c.x, c.y, size, size, ox, oy)
    seam = m.math('MAXIMUM', ramp(m, m.math('ABSOLUTE', sub(m, lu, 0.5)), 0.4975, 0.4995), ramp(m, m.math('ABSOLUTE', sub(m, lv, 0.5)), 0.4975, 0.4995))
    fl1 = ramp(m, noise(m, c.P, scale=1500.0, detail=0.0), 0.66, 0.68)
    fl2 = ramp(m, noise(m, vec(m, c.y, c.x, 2.0), scale=1400.0, detail=0.0), 0.68, 0.70)
    fl3 = ramp(m, noise(m, vec(m, c.x, c.y, 5.0), scale=1700.0, detail=0.0), 0.69, 0.71)
    gran = noise(m, c.P, scale=1800.0, detail=1.0)
    col = m.mix(mul(m, gran, 0.4), c.col, lighten(m, c.col, 0.06))
    col = m.mix(mul(m, h, 0.15), col, lighten(m, c.col, 0.04))
    col = m.mix(fl1, col, H(p.get('fleck1', 0x4b6f9a)))
    col = m.mix(fl2, col, H(p.get('fleck2', 0x8a8f94)))
    col = m.mix(fl3, col, H(p.get('fleck3', 0x9a5a34)))
    logo = p.get('logo')
    if logo:
        path, (lx0, ly0, lx1, ly1) = logo
        uvv = vec(m, m.math('DIVIDE', sub(m, c.x, lx0), lx1 - lx0), m.math('DIVIDE', sub(m, c.y, ly0), ly1 - ly0), 0.0)
        img = m.image(path, vector=uvv, extension='CLIP')
        lm = mul(m, img.outputs['Alpha'], 1.0)
        lcol = m.mix(mul(m, fl1, 0.5), img.outputs['Color'], darken(m, img.outputs['Color'], 0.7))
        lcol = m.mix(mul(m, gran, 0.3), lcol, darken(m, img.outputs['Color'], 0.85))
        col = m.mix(lm, col, lcol)
    else:
        lm = 0.0
    wear = None
    for (pts, width) in p.get('paths', []):
        for (ax, ay), (bxp, byp) in zip(pts, pts[1:]):
            d = seg_dist(m, c.x, c.y, ax, ay, bxp, byp)
            w_ = smooth_less(m, d, width * 0.5, width * 0.5)
            wear = w_ if wear is None else m.math('MAXIMUM', wear, w_)
    wear = 0.0 if wear is None else mul(m, wear, ramp(m, noise(m, c.P, scale=2.0, detail=4.0), 0.2, 0.6))
    chalk = mul(m, ramp(m, noise(m, c.P, scale=3.0, detail=5.0), 0.62, 0.75), p.get('chalk', 0.5))
    sweat = ramp(m, noise(m, c.P, scale=1.2, detail=4.0), 0.66, 0.72)
    col = m.mix(mul(m, wear, 0.35), col, H(0x4a4a48))
    col = m.mix(mul(m, chalk, 0.35), col, H(0xc8c6c0))
    col = m.mix(mul(m, seam, 0.6), col, H(0x090909))
    rough = add(m, 0.86, mul(m, wear, -0.1), mul(m, sweat, -0.25), mul(m, chalk, 0.05))
    return col, rough, 0.0, add(m, mul(m, seam, -0.5), mul(m, gran, 0.05), mul(m, lm, 0.01))


def f_mosaic(c):
    """Unglazed ceramic mosaic (1 in hexagons? no: 1 in squares) on the entry recess floor, the old store's name in
    it (kit.params['mosaic']: image, rect, tile): every tessera takes one color from the image at its center."""
    m = c.m
    p = c.kit.params.get('mosaic', {})
    t = p.get('tile', 0.0254)
    path, (lx0, ly0, lx1, ly1) = p['image']
    lu, lv, h = cell2(m, c.x, c.y, t, t, lx0, ly0)
    cxq = add(m, mul(m, m.math('FLOOR', m.math('DIVIDE', sub(m, c.x, lx0), t)), t), t * 0.5)
    cyq = add(m, mul(m, m.math('FLOOR', m.math('DIVIDE', sub(m, c.y, ly0), t)), t), t * 0.5)
    uvv = vec(m, m.math('DIVIDE', cxq, lx1 - lx0), m.math('DIVIDE', cyq, ly1 - ly0), 0.0)
    img = m.image(path, vector=uvv, extension='EXTEND')
    img.interpolation = 'Closest'
    grout = m.math('MAXIMUM', ramp(m, m.math('ABSOLUTE', sub(m, lu, 0.5)), 0.43, 0.48), ramp(m, m.math('ABSOLUTE', sub(m, lv, 0.5)), 0.43, 0.48))
    col = m.mix(mul(m, h, 0.18), img.outputs['Color'], darken(m, img.outputs['Color'], 0.8))
    dirt = ramp(m, noise(m, c.P, scale=3.0, detail=4.0), 0.4, 0.8)
    col = m.mix(mul(m, dirt, 0.35), col, H(0x4a443a))
    col = m.mix(mul(m, grout, 0.75), col, m.mix(dirt, H(0x8a8272), H(0x4a443a)))
    missing = ramp(m, h, 0.996, 0.998)
    col = m.mix(mul(m, missing, mul(m, sub(m, 1.0, grout), 1.0)), col, H(0x2a2620))
    rough = add(m, 0.55, mul(m, grout, 0.35), mul(m, dirt, 0.1))
    return col, rough, 0.0, add(m, mul(m, grout, -0.5), mul(m, missing, -0.6))


def noise_vec(m, v, scale):
    n = m.node('ShaderNodeTexNoise', Scale=scale, Detail=2.0)
    m.link(v, n.inputs['Vector'])
    return n.outputs['Color']


def add_vec(m, a, b, k=0.05):
    sc = m.node('ShaderNodeVectorMath')
    sc.operation = 'SCALE'
    m.link(b, sc.inputs[0])
    sc.inputs['Scale'].default_value = k
    ad = m.node('ShaderNodeVectorMath')
    ad.operation = 'ADD'
    m.link(a, ad.inputs[0])
    m.link(sc.outputs[0], ad.inputs[1])
    return ad.outputs[0]


FINISH = {'steel': f_steel, 'chrome': f_chrome, 'alu': f_alu, 'bronze': f_bronze, 'powder': f_powder, 'paint': f_paint,
          'wallpaint': f_wallpaint, 'brickpaint': f_brickpaint, 'wood': f_wood, 'laminate': f_laminate, 'particle': f_particle,
          'plywood': f_plywood, 'carpet': f_carpet, 'cloth': f_cloth, 'vinyl': f_vinyl, 'leather': f_leather, 'rubber': f_rubber,
          'plastic': f_plastic, 'gloss': f_gloss, 'paper': f_paper, 'cardboard': f_cardboard, 'cork': f_cork, 'blackglass': f_blackglass,
          'enamel': f_enamel, 'velvet': f_velvet, 'brass': f_brass, 'foam': f_foam, 'ceiltile': f_ceiltile, 'vct': f_vct,
          'rubberfloor': f_rubberfloor, 'mosaic': f_mosaic, 'concrete': f_concrete, 'slatwall': f_slatwall, 'pegboard': f_pegboard,
          'galv': f_galv, 'chalk': f_chalk, 'glassbaked': f_glassbaked, 'prism': f_prism, 'felt': f_felt, 'canvas': f_canvas,
          'lino': f_lino, 'knurl': f_knurl, 'corrugated': f_corrugated}
NO_DUST = {'cloth', 'vct', 'rubberfloor', 'mosaic', 'carpet', 'foam', 'chalk', 'velvet'}


def body_material(kit, group='body'):
    m = core.Mat(f'{kit.prefix}_{group}')
    c = Ctx(m, kit)
    col, rough, metal, h = H(0x808080), 0.5, 0.0, 0.0
    kinds = kit.kinds.get(group, set())
    for kind in sorted(kinds, key=FIN.get):
        fc, fr, fm, fh = FINISH[kind](c)
        if len(kinds) == 1:
            col, rough, metal, h = fc, fr, fm, fh
            continue
        mk = is_k(m, c.fin, FIN[kind])
        col = m.mix(mk, col, fc)
        rough = mix_float(m, mk, rough, fr)
        metal = mix_float(m, mk, metal, fm)
        h = mix_float(m, mk, h, fh)
    # Wear every finish shares: grime in the crevices, rising from the floor, hand grime, dust on top.
    if kit.cavity_amt > 0:
        ao = m.node('ShaderNodeAmbientOcclusion', Distance=kit.cavity)
        ao.only_local = True
        ao.samples = 8
        cav = ramp(m, ao.outputs['AO'], 0.85, 0.4)
        # crevice grime darkens what it's in (a fixed brown would lighten black steel and rubber)
        col = m.mix(mul(m, cav, kit.cavity_amt), col, m.mix(1.0, col, (0.42, 0.39, 0.35, 1.0), blend='MULTIPLY'))
        rough = add(m, rough, mul(m, cav, 0.06))
    if kit.low > 0:
        breakup = ramp(m, noise(m, c.P, scale=7.0, detail=4.0), 0.3, 0.7)
        low = mul(m, ramp(m, c.z, kit.low, 0.0), breakup)
        if kinds & {'vct', 'rubberfloor', 'mosaic', 'carpet'}:
            low = mul(m, low, ramp(m, c.nz, 0.9, 0.5))   # floors aren't grimed by themselves
        col = m.mix(mul(m, low, 0.45), col, m.mix(0.5, H(0x3a342c), m.mix(1.0, col, (0.5, 0.47, 0.42, 1.0), blend='MULTIPLY')))
        rough = mix_float(m, mul(m, low, 0.6), rough, 0.85)
        metal = mix_float(m, mul(m, low, 0.5), metal, 0.0)
    for (cx, cy, cz), rad, amt in kit.grime:
        dx, dy, dz = sub(m, c.x, cx), sub(m, c.y, cy), sub(m, c.z, cz)
        d = m.math('SQRT', add(m, mul(m, dx, dx), mul(m, dy, dy), mul(m, dz, dz)))
        g = mul(m, mul(m, smooth_less(m, d, rad * 0.6, rad * 0.4), ramp(m, noise(m, c.P, scale=35.0, detail=3.0), 0.3, 0.7)), amt)
        col = m.mix(mul(m, g, 0.7), col, H(0x332d24))
        rough = mix_float(m, g, rough, 0.62)
    for (cx, cy, cz), rad, amt in kit.polish:
        dx, dy, dz = sub(m, c.x, cx), sub(m, c.y, cy), sub(m, c.z, cz)
        d = m.math('SQRT', add(m, mul(m, dx, dx), mul(m, dy, dy), mul(m, dz, dz)))
        g = mul(m, mul(m, smooth_less(m, d, rad * 0.55, rad * 0.45), ramp(m, noise(m, c.P, scale=18.0, detail=3.0), 0.25, 0.65)), amt)
        col = m.mix(mul(m, g, 0.12), col, darken(m, col, 0.8))
        rough = mix_float(m, g, rough, 0.24)
    if kit.dust > 0.0 and not kinds <= NO_DUST:
        metal_less = sub(m, 1.0, mul(m, metal, 0.6)) if not isinstance(metal, (int, float)) else 1.0 - 0.6 * metal
        up = mul(m, ramp(m, c.nz, 0.6, 0.95), mul(m, ramp(m, noise(m, c.P, scale=22.0, detail=4.0), 0.4, 0.8), kit.dust))
        up = mul(m, up, metal_less)
        for k in kinds & NO_DUST:
            up = mul(m, up, sub(m, 1.0, is_k(m, c.fin, FIN[k])))
        col = m.mix(up, col, H(0x9a958b))
        rough = mix_float(m, up, rough, 0.9)
        metal = mix_float(m, up, metal, 0.0)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    bump = m.node('ShaderNodeBump', Strength=1.0, Distance=0.001)
    m.link(h if not isinstance(h, (int, float)) else m.math('ADD', h, 0.0), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
    if kinds & {'cloth', 'paper', 'cardboard', 'velvet', 'felt', 'canvas', 'carpet', 'foam', 'ceiltile', 'chalk'} == kinds:
        m.set('Specular IOR Level', 0.3)
    return m


def print_material(kit):
    """The atlas through the decals' UVs: its color and finish, a film of grime; lit panels glow."""
    A = kit.atlas
    m = core.Mat(f'{kit.prefix}_print')
    uvn = m.node('ShaderNodeUVMap')
    uvn.uv_map = 'UVMap'
    img = m.image(A.color, vector=uvn.outputs['UV'], extension='EXTEND')
    surf = m.image(A.surface, non_color=True, vector=uvn.outputs['UV'], extension='EXTEND')
    ss = m.node('ShaderNodeSeparateColor')
    m.link(surf.outputs['Color'], ss.inputs['Color'])
    tc, sep = object_coords(m)
    grime = ramp(m, noise(m, tc.outputs['Object'], scale=25.0, detail=4.0), 0.55, 0.85)
    col = m.mix(mul(m, grime, 0.14), img.outputs['Color'], H(0x3a342c))
    m.set('Base Color', col)
    m.set('Roughness', add(m, ss.outputs['Green'], mul(m, grime, 0.08)))
    m.set('Metallic', ss.outputs['Red'])
    lit = [(k, p['glow']) for k, p in A.panels.items() if p['glow'] > 0.0]
    if lit:
        gmax = max(g for _, g in lit)
        us = m.node('ShaderNodeSeparateXYZ')
        m.link(uvn.outputs['UV'], us.inputs['Vector'])
        u, v = us.outputs['X'], us.outputs['Y']
        mask = None
        for k, g in lit:
            u0, v0, u1, v1 = A.rects[k]
            r = mul(m, mul(m, m.math('GREATER_THAN', u, u0), m.math('LESS_THAN', u, u1)), mul(m, m.math('GREATER_THAN', v, v0), m.math('LESS_THAN', v, v1)))
            r = mul(m, r, g / gmax)
            mask = r if mask is None else m.math('MAXIMUM', mask, r)
        m.set('Emission Color', m.mix(1.0, img.outputs['Color'], vec(m, mask, mask, mask), blend='MULTIPLY'))
        m.set('Emission Strength', gmax)
    return m


# ================================================================== bake split (unbaked slots ride along)

_SPLIT = {}
_SKIPPED = []


def _delete_faces(obj, test):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if test(f)], context='FACES')
    bm.to_mesh(obj.data)
    bm.free()


def _prune_slots(obj):
    me = obj.data
    fi = np.zeros(len(me.polygons), dtype=np.int32)
    me.polygons.foreach_get('material_index', fi)
    used = sorted(set(fi.tolist()))
    mats = [obj.material_slots[i].material for i in used]
    remap = {old: new for new, old in enumerate(used)}
    me.materials.clear()
    for mt in mats:
        me.materials.append(mt)
    me.polygons.foreach_set('material_index', np.array([remap[i] for i in fi], dtype=np.int32))


def is_unbaked_mat(mt):
    return mt is not None and (mt.name.startswith(UNBAKED_PREFIXES) or mt.name.startswith('glow_'))


def reuse_bake(o, double_sided=False):
    """NS_REUSE=<mesh,...>: give a mesh whose geometry and UVs haven't changed since its last bake the textures that
    bake left in art/build/textures/<mesh>/ instead of baking it again (the GPU is shared). True if it could."""
    slots = [s.material for s in o.material_slots]
    baked = [i for i, mt in enumerate(slots) if not is_unbaked_mat(mt)]
    tex_dir = os.path.join(core.BUILD_DIR, 'textures', o.name)
    new = {}
    for j, i in enumerate(baked):
        suffix = f'_{j}' if len(baked) > 1 else ''
        p = {key: os.path.join(tex_dir, f'T_{o.name}{suffix}_{key}.png') for key in ('BaseColor', 'ORM', 'Normal', 'Emissive')}
        if not all(os.path.exists(p[key]) for key in ('BaseColor', 'ORM', 'Normal')):
            return False
        bsdf = next(n for n in slots[i].node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
        strength = bsdf.inputs['Emission Strength'].default_value
        spec_in = bsdf.inputs['Specular IOR Level']
        specular = 0.5 if spec_in.is_linked else spec_in.default_value
        em = p['Emissive'] if os.path.exists(p['Emissive']) else None
        new[i] = core.gltf_material(f'M_{o.name}{suffix}', p['BaseColor'], p['ORM'], p['Normal'], em, strength, specular)
        new[i].use_backface_culling = not double_sided
    for i, mt in new.items():
        o.material_slots[i].material = mt
    print(f'[{MODULE}] {o.name}: reused its last bake')
    return True


def bake_parts(objs, double_sided=False):
    """Meshes that are wholly unbaked (glass, glows) skip the bake; a baked mesh with unbaked slots (a door's glass, a
    screen, a mirror) bakes without them: they're split off, hidden from the bake (so they don't shade what's behind
    them) and joined back after it, in their slots' places. NS_REUSE names meshes to take their last bake's textures."""
    _SPLIT.clear()
    _SKIPPED.clear()
    reuse = {s.strip() for s in os.environ.get('NS_REUSE', '').split(',') if s.strip()}
    out = []
    for o in objs:
        slots = [s.material for s in o.material_slots]
        if all(is_unbaked_mat(mt) for mt in slots):
            o.hide_render = True
            _SKIPPED.append(o)
            continue
        if o.name in reuse and reuse_bake(o, double_sided):
            o.hide_render = True
            _SKIPPED.append(o)
            continue
        idx = [i for i, mt in enumerate(slots) if is_unbaked_mat(mt)]
        if idx:
            dup = o.copy()
            dup.data = o.data.copy()
            core.link(dup)
            dup.name = o.name + '__unbaked'
            _SPLIT[o.name] = (dup, slots)
            _delete_faces(o, lambda f: f.material_index in idx)
            _delete_faces(dup, lambda f: f.material_index not in idx)
            _prune_slots(o)
            _prune_slots(dup)
            dup.hide_render = True
        out.append(o)
    return out


def assemble_core(objs, names):
    """Joins each split mesh back together with its slots in their first order; returns the meshes in `names` order."""
    by = {o.name: o for o in objs}
    for o in _SKIPPED:
        by[o.name] = o
    for name, (dup, slots) in _SPLIT.items():
        o = by.get(name)
        if o is None:
            continue
        dup.hide_render = False
        baked = [s.material for s in o.material_slots]
        unb = [s.material for s in dup.material_slots]
        j = core.join([o, dup], name)
        j.name = name
        j.data.name = name
        # baked materials were renamed M_<mesh>_<i>: keep the original order, baked ones by position
        order = []
        bi = iter(baked)
        for mt in slots:
            order.append(mt if is_unbaked_mat(mt) else next(bi))
        order_slots(j, order)
        by[name] = j
    for o in by.values():
        o.hide_render = False
    return [by[n] for n in names if n in by]


# ================================================================== review: lights, mock street, shots

def review_mat(name, hexc, rough=0.5, metal=0.0):
    m = core.Mat(name)
    m.set('Base Color', H(hexc))
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    return m


def area_light(name, loc, size, energy, color=(1, 1, 1), rot=(0.0, 0.0, 0.0), shape='RECTANGLE', size_y=None, spread=180.0):
    d = bpy.data.lights.new(name, 'AREA')
    d.shape = shape
    d.size = size
    if size_y is not None:
        d.size_y = size_y
    d.energy = energy
    d.color = color
    try:
        d.spread = math.radians(spread)
    except AttributeError:
        pass
    o = core.link(bpy.data.objects.new(name, d))
    o.location = loc
    o.rotation_euler = rot
    o.visible_camera = False
    return o


def spot_light(name, loc, target, energy, color=(1, 1, 1), angle=60.0, blend=0.4, radius=0.05):
    d = bpy.data.lights.new(name, 'SPOT')
    d.energy = energy
    d.color = color
    d.spot_size = math.radians(angle)
    d.spot_blend = blend
    d.shadow_soft_size = radius
    o = core.link(bpy.data.objects.new(name, d))
    o.location = loc
    dd = Vector(target) - Vector(loc)
    o.rotation_euler = dd.to_track_quat('-Z', 'Y').to_euler()
    return o


def point_light(name, loc, energy, color=(1, 1, 1), radius=0.05):
    d = bpy.data.lights.new(name, 'POINT')
    d.energy = energy
    d.color = color
    d.shadow_soft_size = radius
    o = core.link(bpy.data.objects.new(name, d))
    o.location = loc
    return o


def kelvin(k):
    """An approximate linear RGB for a color temperature (1000..12000 K), normalized to its max channel."""
    t = k / 100.0
    if t <= 66:
        r = 255.0
        g = 99.4708025861 * math.log(t) - 161.1195681661
        b = 0.0 if t <= 19 else 138.5177312231 * math.log(t - 10) - 305.0447927307
    else:
        r = 329.698727446 * (t - 60) ** -0.1332047592
        g = 288.1221695283 * (t - 60) ** -0.0755148492
        b = 255.0
    rgb = [core.srgb_to_linear(max(0.0, min(255.0, v)) / 255.0) for v in (r, g, b)]
    mx = max(rgb)
    return tuple(v / mx for v in rgb)


def review_glass():
    """A real glass for the review renders (the exported ns_glass is the game's stand-in)."""
    m = bpy.data.materials.get('rv_glass')
    if m:
        return m
    g = core.Mat('rv_glass')
    g.set('Base Color', (0.92, 0.95, 0.94, 1.0))
    g.set('Roughness', 0.02)
    g.set('Transmission Weight', 1.0)
    g.set('IOR', 1.5)
    try:
        g.m.surface_render_method = 'DITHERED'
    except (AttributeError, TypeError):
        pass
    return g.m


def review_haze(src):
    """A review stand-in for a [runtime] haze sheet: mostly transparent, a little white where its mask says dust."""
    m = bpy.data.materials.get('rv_haze')
    if m is not None:
        return m
    img = next((n.image for n in (src.node_tree.nodes if src.node_tree else []) if n.type == 'TEX_IMAGE' and n.image), None)
    m = bpy.data.materials.new('rv_haze')
    try:
        m.use_nodes = True
    except (AttributeError, TypeError):
        pass
    nt = m.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    tr = nt.nodes.new('ShaderNodeBsdfTransparent')
    pb = nt.nodes.new('ShaderNodeBsdfPrincipled')
    pb.inputs['Base Color'].default_value = (0.9, 0.9, 0.88, 1.0)
    pb.inputs['Roughness'].default_value = 1.0
    mix = nt.nodes.new('ShaderNodeMixShader')
    mul_ = nt.nodes.new('ShaderNodeMath')
    mul_.operation = 'MULTIPLY'
    mul_.inputs[1].default_value = 0.10
    if img is not None:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = img
        nt.links.new(tex.outputs['Color'], mul_.inputs[0])
    else:
        mul_.inputs[0].default_value = 0.5
    nt.links.new(mul_.outputs[0], mix.inputs[0])
    nt.links.new(tr.outputs[0], mix.inputs[1])
    nt.links.new(pb.outputs[0], mix.inputs[2])
    nt.links.new(mix.outputs[0], out.inputs['Surface'])
    return m


def swap_for_review(objs, glow_colors):
    """Unbaked slots get review stand-ins: ns_glass becomes real glass; glow meshes emit in their family's color."""
    saved = []
    for o in objs:
        for i, s in enumerate(o.material_slots):
            mt = s.material
            if mt is None:
                continue
            if mt.name.startswith('ns_glass'):
                saved.append((o, i, mt))
                s.material = review_glass()
            elif mt.name.startswith('ns_runtime_haze'):
                saved.append((o, i, mt))
                s.material = review_haze(mt)
            elif mt.name.startswith('glow_'):
                fam = next((f for f in glow_colors if f.lower() in o.name.lower()), None)
                if fam:
                    col, strength = glow_colors[fam]
                    gm = bpy.data.materials.get(f'rv_glow_{fam}')
                    if gm is None:
                        g = core.Mat(f'rv_glow_{fam}')
                        g.set('Base Color', (1, 1, 1, 1))
                        g.set('Emission Color', (*col, 1.0))
                        g.set('Emission Strength', strength)
                        gm = g.m
                    saved.append((o, i, mt))
                    s.material = gm
    return saved


def restore_swaps(saved):
    for o, i, mt in saved:
        try:
            o.material_slots[i].material = mt
        except ReferenceError:
            pass


def shot(cam, path, loc, target, lens, w=960, h=640, samples=64, exposure=0.0):
    sc = bpy.context.scene
    cam.location = Vector(loc)
    d = Vector(target) - cam.location
    cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    cam.data.lens = lens
    sc.render.resolution_x, sc.render.resolution_y = w, h
    sc.render.resolution_percentage = 100
    sc.cycles.samples = samples
    sc.cycles.use_denoising = True
    sc.cycles.adaptive_threshold = 0.03
    sc.render.film_transparent = False
    sc.view_settings.exposure = exposure
    sc.render.filepath = path
    sc.render.image_settings.file_format = 'PNG'
    sc.render.image_settings.color_mode = 'RGB'
    bpy.ops.render.render(write_still=True)
    return path


def compose(name, paths, cols, label=None, module=None):
    imgs = [review._pixels(p) for p in paths]
    h, w = imgs[0].shape[:2]
    rows = (len(imgs) + cols - 1) // cols
    gap = 6
    W = cols * w + (cols - 1) * gap
    Hh = rows * h + (rows - 1) * gap
    out = np.zeros((Hh, W, 4), np.float32)
    out[:, :, :3] = 0.05
    out[:, :, 3] = 1.0
    for i, im in enumerate(imgs):
        r, cc = i // cols, i % cols
        y0 = Hh - (r + 1) * h - r * gap
        x0 = cc * (w + gap)
        ih, iw = im.shape[:2]
        out[y0:y0 + min(h, ih), x0:x0 + min(w, iw)] = im[:min(h, ih), :min(w, iw)]
    path = os.path.join(core.REVIEW_DIR, f'{name}.jpg')
    img = bpy.data.images.new(f'rv_{name}', W, Hh, alpha=False)
    img.pixels.foreach_set(out.ravel())
    img.filepath_raw = path
    img.file_format = 'JPEG'
    img.save(quality=90)
    bpy.data.images.remove(img)
    print(f'[{module or MODULE}] review sheet {os.path.relpath(path, core.ROOT)}')
    return path


def scale_figure(loc, yaw=0.0, height=1.75):
    """A 175 cm grey stand-in person for scale in the review renders (head, torso, arms, legs)."""
    parts_ = []
    k = height / 1.75
    parts_.append(lathe_axis('fig_head', [(0.0, 1.53), (0.07, 1.56), (0.095, 1.63), (0.085, 1.70), (0.05, 1.745), (0.0, 1.75)]))
    parts_.append(lathe_axis('fig_body', [(0.0, 0.78), (0.17, 0.82), (0.16, 1.0), (0.15, 1.2), (0.21, 1.40), (0.12, 1.47), (0.05, 1.5), (0.0, 1.52)]))
    for s in (-1, 1):
        parts_.append(tube('fig_leg', [(0.0, s * 0.095, 0.82), (0.0, s * 0.10, 0.45), (0.02, s * 0.10, 0.06)], 0.07, 12))
        parts_.append(bxc('fig_foot', (-0.06, s * 0.10, 0.04), (0.26, 0.10, 0.08), 0.03))
        parts_.append(tube('fig_arm', [(0.0, s * 0.21, 1.40), (0.02, s * 0.25, 1.12), (-0.04, s * 0.25, 0.86)], 0.045, 10))
    for p in parts_:
        p.data.transform(Matrix.Diagonal((1.0, 1.0, k, 1.0)))
    o = core.join(parts_, 'rv_figure')
    o.data.shade_smooth()
    core.assign(o, review_mat('rv_figure', 0x6f7378, 0.6))
    o.location = loc
    o.rotation_euler = (0.0, 0.0, math.radians(yaw))
    return o


def tinted(mat, rgb, name=None):
    """A copy of a baked material with its base color multiplied by rgb (the game's [tint] through custom data)."""
    m = mat.copy()
    m.name = name or f'{mat.name}_tint'
    nt = m.node_tree
    bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
    sock = bsdf.inputs['Base Color']
    if sock.is_linked:
        src = sock.links[0].from_socket
        mix = nt.nodes.new('ShaderNodeMix')
        mix.data_type = 'RGBA'
        mix.blend_type = 'MULTIPLY'
        mix.inputs[0].default_value = 1.0
        nt.links.new(src, mix.inputs[6])
        mix.inputs[7].default_value = (rgb[0], rgb[1], rgb[2], 1.0)
        nt.links.new(mix.outputs[2], sock)
    else:
        c = sock.default_value
        sock.default_value = (c[0] * rgb[0], c[1] * rgb[1], c[2] * rgb[2], 1.0)
    return m


def tinted_copy(obj, rgb, slot=0, matrix=None):
    d = obj.copy()
    d.data = obj.data.copy()
    core.link(d)
    d.data.materials[slot] = tinted(obj.data.materials[slot], rgb)
    if matrix is not None:
        d.matrix_world = matrix
    d.hide_render = False
    return d


def import_glb(name):
    """A mesh from unreal/Art/Meshes (an earlier build's export), for dressing the review; None if missing."""
    path = os.path.join(core.EXPORT_DIR, f'{name}.glb')
    if not os.path.exists(path):
        return None
    # The bake's own 'glTF Material Output' group (occlusion only) trips Blender 5.2's importer, which expects its own.
    g = bpy.data.node_groups.get('glTF Material Output')
    if g is not None and 'Iridescence Factor' not in [s.name for s in g.interface.items_tree]:
        g.name = 'glTF Material Output (bake)'
    before = set(bpy.data.objects)
    try:
        bpy.ops.import_scene.gltf(filepath=path)
    except RuntimeError as exc:
        print(f'[{MODULE}] could not import {name} for the review: {exc}')
        return None
    new = [o for o in bpy.data.objects if o not in before]
    mesh = next((o for o in new if o.type == 'MESH'), None)
    for o in new:
        if o is not mesh:
            bpy.data.objects.remove(o)
    if mesh is not None:
        mesh.parent = None
        mesh.matrix_world = Matrix()
        mesh.name = f'rv_{name}'
    return mesh


class ReviewRoom:
    """A neutral shop corner for close looks: a floor, a wall behind (optional), fluorescent tubes or high-bays."""

    def __init__(self, floor_hex=0x8d8a7b, wall_hex=0xd8d2bc, kelvin_k=4000, energy=70.0, world=(0.03, 0.032, 0.036)):
        self.temp = []
        fm = core.Mat('rv_floor')
        tc = fm.node('ShaderNodeTexCoord')
        ch = fm.node('ShaderNodeTexChecker', Scale=1.0 / 0.3048 * 0.5 * 2.0)
        ch.inputs['Color1'].default_value = H(floor_hex)
        ch.inputs['Color2'].default_value = H(_tone(floor_hex, 0.92))
        fm.link(tc.outputs['Object'], ch.inputs['Vector'])
        n = noise(fm, tc.outputs['Object'], scale=3.0, detail=4.0)
        fm.set('Base Color', fm.mix(mul(fm, n, 0.2), ch.outputs['Color'], H(_tone(floor_hex, 0.7))))
        fm.set('Roughness', add(fm, 0.35, mul(fm, n, 0.2)))
        bpy.ops.mesh.primitive_plane_add(size=1.0)
        self.floor = bpy.context.view_layer.objects.active
        self.floor.scale = (40.0, 40.0, 1.0)
        self.floor.name = 'rv_floor'
        core.assign(self.floor, fm)
        bpy.ops.mesh.primitive_plane_add(size=1.0, rotation=(0.0, math.radians(90.0), 0.0))
        self.wall = bpy.context.view_layer.objects.active
        self.wall.scale = (8.0, 40.0, 1.0)
        self.wall.name = 'rv_wall'
        core.assign(self.wall, review_mat('rv_wall', wall_hex, 0.85))
        self.temp += [self.floor, self.wall]
        col = kelvin(kelvin_k)
        for x in (-2.4, -0.6, 1.2):
            for y in (-1.5, 0.3, 2.1):
                self.temp.append(area_light('rv_tube', (x, y, 3.2), 1.2, energy, col, size_y=0.3))
        self.temp.append(area_light('rv_fill', (-4.0, -3.0, 2.0), 3.0, 120.0, (0.85, 0.9, 1.0), rot=(math.radians(70), 0, math.radians(-55))))
        bpy.context.scene.world.color = world
        cd = bpy.data.cameras.new('rv_cam')
        cd.clip_start = 0.02
        cd.clip_end = 200.0
        self.cam = core.link(bpy.data.objects.new('rv_cam', cd))
        bpy.context.scene.camera = self.cam
        self.temp.append(self.cam)

    def wall_at(self, x):
        if x is None:
            self.wall.hide_render = True
        else:
            self.wall.hide_render = False
            self.wall.location = (x, 0.0, 4.0)

    def close(self):
        for o in self.temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        bpy.context.scene.camera = None


def grid_layout(objs, place=None, row_w=6.0, gap=0.45):
    """Stands props in rows (along Y, rows stepping back in +X) for the standard sheet; returns REVIEW_VIEWS framing them
    from the front (-X)."""
    place = place or {}
    x = y = row_d = 0.0
    for o in objs:
        o.matrix_world = place.get(o.name, Matrix())
        bpy.context.view_layer.update()
        lo, hi = core.bounds([o])
        w, d = hi.y - lo.y, hi.x - lo.x
        if y > 0 and y + w > row_w:
            x += row_d + gap
            y = row_d = 0.0
        o.matrix_world = Matrix.Translation((x - lo.x, y - lo.y, 0.0)) @ place.get(o.name, Matrix())
        y += w + gap
        row_d = max(row_d, d)
    bpy.context.view_layer.update()
    lo, hi = core.bounds(objs)
    c = (lo + hi) / 2
    size = (hi - lo).length
    span = max(hi.y - lo.y, 1.0)
    f = span / size * 2.3
    return [('front', -90, 16, f, (c.x, c.y, 0.8)), ('angle', -55, 28, f * 0.95, (c.x, c.y, 0.7)),
            ('detail', -70, 14, f * 0.42, (lo.x + 0.5, lo.y + 1.6, 0.9))]


def _tone(h, f):
    r, g, b = (h >> 16) & 255, (h >> 8) & 255, h & 255
    return (min(255, int(r * f)) << 16) | (min(255, int(g * f)) << 8) | min(255, int(b * f))


def detail_review(objs, shots, sheet, module, dress=None, wall=None, room=None, cols=3, size=(640, 640), samples=40, place=None):
    """Close looks at each mesh (at the origin, the others hidden): shots {mesh: [(camera, target, lens)]} in the mesh's
    frame; dress(name) -> extra objects shown with it (garments on a rack); wall {mesh: x of the wall plane or None}."""
    sc = bpy.context.scene
    saved = {o.name: (o.matrix_world.copy(), o.hide_render) for o in objs}
    room = room or ReviewRoom()
    tmp_dir = os.path.join(core.BUILD_DIR, 'review', sheet)
    os.makedirs(tmp_dir, exist_ok=True)
    paths = []
    try:
        for o in objs:
            if o.name not in shots:
                continue
            for p in objs:
                p.hide_render = p is not o
            o.matrix_world = (place or {}).get(o.name, Matrix())
            extra = dress(o.name) if dress else []
            bpy.context.view_layer.update()
            lo, hi = core.bounds([o])
            wx = (wall or {}).get(o.name, hi.x + 0.03)
            if wx is not None and any(loc[0] > wx - 0.05 for loc, _, _ in shots[o.name]):
                wx = None      # a camera behind the wall plane: leave the wall out
            room.wall_at(wx)
            for i, (loc, tgt, lens) in enumerate(shots[o.name]):
                paths.append(shot(room.cam, os.path.join(tmp_dir, f'{o.name}_{i}.png'), loc, tgt, lens, size[0], size[1], samples))
            for e in extra:
                try:
                    bpy.data.objects.remove(e)
                except ReferenceError:
                    pass
        if paths:
            compose(sheet, paths, cols, module=module)
    finally:
        room.close()
        for n, (mw, hid) in saved.items():
            ob = bpy.data.objects.get(n)
            if ob is not None:
                ob.matrix_world, ob.hide_render = mw, hid


def world_bg(color, strength=1.0):
    """The review world's background (Blender 5's worlds always shade through nodes, so world.color alone is ignored)."""
    w = bpy.context.scene.world
    try:
        w.use_nodes = True
    except (AttributeError, TypeError):
        pass
    w.color = color
    nt = w.node_tree
    if nt is None:
        return
    bg = next((n for n in nt.nodes if n.type == 'BACKGROUND'), None) or nt.nodes.new('ShaderNodeBackground')
    out = next((n for n in nt.nodes if n.type == 'OUTPUT_WORLD'), None) or nt.nodes.new('ShaderNodeOutputWorld')
    if not bg.outputs['Background'].is_linked:
        nt.links.new(bg.outputs['Background'], out.inputs['Surface'])
    bg.inputs['Color'].default_value = (color[0], color[1], color[2], 1.0)
    bg.inputs['Strength'].default_value = strength


class StreetMock:
    """The street around a shop front for the review: a brick facade with the shop's opening left clear, the sidewalk
    and curb, the road; a night (LED lamps, a wet road) or an overcast day."""

    def __init__(self, front_y, holes, x_range, top=8.0, sidewalk=4.5, road=12.0, facing=1.0, brick=0x7e4232):
        self.temp = []
        fy = front_y
        bm = core.Mat('rv_brick')
        tc = bm.node('ShaderNodeTexCoord')
        br = bm.node('ShaderNodeTexBrick', Scale=1.0, **{'Mortar Size': 0.01, 'Brick Width': 0.2032, 'Row Height': 0.0675})
        br.inputs['Color1'].default_value = H(brick)
        br.inputs['Color2'].default_value = H(_tone(brick, 0.82))
        br.inputs['Mortar'].default_value = H(0x7d776c)
        sepn = bm.node('ShaderNodeSeparateXYZ')
        bm.link(tc.outputs['Object'], sepn.inputs['Vector'])
        bm.link(vec(bm, add(bm, sepn.outputs['X'], sepn.outputs['Y']), sepn.outputs['Z'], 0.0), br.inputs['Vector'])
        n = noise(bm, tc.outputs['Object'], scale=2.0, detail=4.0)
        bm.set('Base Color', bm.mix(mul(bm, n, 0.3), br.outputs['Color'], H(0x3a2a22)))
        bm.set('Roughness', 0.85)
        bump = bm.node('ShaderNodeBump', Strength=0.4, Distance=0.004)
        bm.link(br.outputs['Fac'], bump.inputs['Height'])
        bm.set('Normal', bump.outputs['Normal'])
        x0, x1 = x_range
        wall = wall_with_holes('rv_facade', x0, x1, -0.3, top, holes, fy, 0.35, toward=facing)
        core.assign(wall, bm)
        self.temp.append(wall)
        side = 1.0 if facing > 0 else -1.0
        sw = core.Mat('rv_sidewalk')
        nn = noise(sw, sw.node('ShaderNodeTexCoord').outputs['Object'], scale=1.5, detail=5.0)
        sw.set('Base Color', sw.mix(mul(sw, nn, 0.4), H(0x8a867e), H(0x5d5a54)))
        sw.set('Roughness', 0.6)
        ya, yb = sorted((fy, fy + side * sidewalk))
        walk = bx('rv_walk', x0, x1, ya, yb, -0.2, 0.0)
        core.assign(walk, sw)
        ysa, ysb = sorted((fy + side * sidewalk, fy + side * (sidewalk + road)))
        self.road = core.Mat('rv_road')
        self.road.set('Base Color', H(0x2a2b2e))
        self.road.set('Roughness', 0.35)
        road_o = bx('rv_road', x0, x1, ysa, ysb, -0.4, -0.15)
        core.assign(road_o, self.road)
        cy = fy + side * sidewalk
        ca, cb = sorted((cy, cy - side * 0.15))
        curb = bx('rv_curb', x0, x1, ca, cb, -0.3, 0.0)
        core.assign(curb, review_mat('rv_curbm', 0x9a958c, 0.7))
        self.temp += [walk, road_o, curb]
        self.lamps = []

    def lamp(self, loc, target, kelvin_k, energy, angle=110.0):
        L = spot_light('rv_street', loc, target, energy, kelvin(kelvin_k), angle, 0.6, 0.15)
        self.lamps.append(L)
        self.temp.append(L)
        return L

    def night(self):
        world_bg((0.004, 0.005, 0.009), 1.0)
        self.road.set('Roughness', 0.12)
        self.road.set('Base Color', H(0x161718))
        for L in self.lamps:
            L.hide_render = False

    def day(self):
        world_bg((0.62, 0.66, 0.74), 3.6)     # an overcast sky, several times the room lights
        self.road.set('Roughness', 0.7)
        self.road.set('Base Color', H(0x2f3033))
        for L in self.lamps:
            L.hide_render = True

    def close(self):
        for o in self.temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        world_bg((0.0508, 0.0508, 0.0508), 1.0)     # back to the default world for the close-look sheets


# ================================================================== Second Chance Thrift

TEXTURE_SIZE = 1024
TEXTURE_SIZES = {
    'thf_body': 4096, 'thf_glass': 2048, 'thf_print': 4096,
    'ths_floor': 4096, 'ths_wall': 4096, 'ths_ceil': 2048, 'ths_trim': 2048, 'ths_print': 2048,
    'thr_body': 1024, 'thk_body': 1024, 'tht_body': 2048, 'tht_print': 512,
    'thw_body': 2048, 'thh_body': 2048, 'thc_body': 2048, 'thc_print': 1024,
    'thb_body': 2048, 'thb_cloth': 2048, 'thb_print': 512, 'thn_body': 2048, 'thn_print': 2048,
    'thg_body': 256, 'thd_body': 1024, 'thd_print': 512, 'thdr_body': 1024, 'thdr_print': 512, 'thbc_cloth': 2048, 'thm_body': 1024, 'thmb_body': 1024, 'thx_body': 4096, 'thx_print': 2048,
    'thp_tee': 1024, 'thp_hoodie': 1024, 'thp_jacket': 1024, 'thp_pants': 1024,
    'thq_tee': 1024, 'thq_jeans': 1024, 'thq_hoodie': 1024,
}
AO_DISTANCE = 0.05
DOUBLE_SIDED = False
REVIEW_SCREEN_LIGHT = False
REVIEW_VIEWS = None   # set by pose_for_review

CLOTH_GREY = 0xc6c5c0   # [tint] cloth bakes light neutral: the game multiplies it by the instance's color

# ------------------------------------------------------------------ the plot (stage cm, spec 2.3 / 2.4)

OPEN_X = (-2440.0, -3660.0)       # the storefront opening in the Y 4950 plane, Z 0..400
OPEN_Y = 4950.0
OPEN_TOP = 400.0
IN_X = (-2460.0, -3640.0)         # the interior's clear box
IN_Y = (3650.0, 4920.0)
CEIL = 330.0                      # the drop ceiling (the structure is at 400)
DOOR_X = (-2960.0, -3140.0)       # the double doors, recessed 1.2 m
DOOR_Y = OPEN_Y - 120.0

# Blender (SITE) versions
OX0, OX1 = -OPEN_X[0] / 100, -OPEN_X[1] / 100      # 24.40 .. 36.60 (x grows east)
IX0, IX1 = -IN_X[0] / 100, -IN_X[1] / 100          # 24.60 .. 36.40
IY0, IY1 = IN_Y[0] / 100, IN_Y[1] / 100            # 36.50 .. 49.20
FRONT_Y = OPEN_Y / 100                             # 49.50
GLASS_Y = FRONT_Y - 0.12                           # 49.38: the display glass's outer face
DOOR_PLANE = DOOR_Y / 100                          # 48.30
DX0, DX1 = -DOOR_X[0] / 100, -DOOR_X[1] / 100      # 29.60 .. 31.40
CZ = CEIL / 100                                    # 3.30
KNEE_Y = 48.40                                     # the display platforms' back (knee) walls
PLAT_Z = 0.50                                      # the bulkheads' and platforms' top
TRANSOM_Z = (2.95, 3.30)                           # the prism-glass transom band (the sign band is 3.30..4.00)
DOOR_H = 2.15                                      # the leaves' height; the transom over them to 2.95
LEAF_W = (DX1 - DX0 - 0.10) / 2                    # each leaf (0.85 m), stiles meeting in the middle
RET = 0.68                                         # the angled glass returns' run in x (each side)
# The drop ceiling's 2 x 4 grid: main tees along Y every 0.61 m (centered, 0.41 m border tiles), cross tees every 1.22 m.
GRID_X = [IX0] + [IX0 + 0.41 + 0.61 * k for k in range(19)] + [IX1]
GRID_Y = [IY0] + [IY0 + 0.33 + 1.22 * k for k in range(11)] + [GLASS_Y - 0.02]
TROFFERS = [(x, y) for x in (IX0 + 0.41 + 0.61 * 4 + 0.305, IX0 + 0.41 + 0.61 * 13 + 0.305) for y in (37.44, 39.88, 42.32, 44.76, 47.20)]
BACK_DOOR = (31.00, 31.92, 2.13)                   # the donation door in the back wall (Blender x0, x1, height)


# ------------------------------------------------------------------ chrome racks

RR_R, RR_Z, RR_T = 0.60, 1.37, 0.016                # the round rack's ring: radius (tube center), height, tube radius
SR_HALF, SR_X, SR_Z, SR_T = 0.90, 0.28, 1.37, 0.0135  # the straight rack: half length, rails at x = +-SR_X, height, radius
HANGER_STEP = 0.06


def round_rack_points():
    """The round rack's hanger points H0..H61: (x, y, z, yaw): on top of the ring, every 6 cm round it from +X
    counter-clockwise; a hung garment (front -X, shoulders along Y) placed there at that yaw hangs radially, its front
    toward the counter-clockwise tangent."""
    n = int(2 * math.pi * RR_R / HANGER_STEP)
    out = []
    for i in range(n):
        a = 2 * math.pi * i / n
        out.append((RR_R * math.cos(a), RR_R * math.sin(a), RR_Z + RR_T, math.degrees(a) - 90.0))
    return out


def straight_rack_points():
    """The straight rack's hanger points H0..H57 (x, y, z, yaw): H0..H28 on the -X rail from -Y to +Y every 6 cm,
    H29..H57 on the +X rail; a hung garment at yaw 90 faces -Y along its rail (shoulders across it)."""
    out = []
    n = int(round(2 * (SR_HALF - 0.06) / HANGER_STEP)) + 1
    for xs in (-SR_X, SR_X):
        for i in range(n):
            out.append((xs, -(SR_HALF - 0.06) + i * HANGER_STEP, SR_Z + SR_T, 90.0))
    return out


def caster(k, x, y, z_top, yaw=0.0, wheel_r=0.032, lite=False):
    """A swivel caster under a leg end: the plate, the fork, the wheel (rubber), the axle. lite: fewer sides (a mesh on
    a tight triangle budget)."""
    sw_seg, wh_seg, ax_seg = (12, 14, 6) if lite else (20, 24, 10)
    m = M(x, y, 0.0, yaw)
    k.add(xf(bxc('cst_plate', (0.0, 0.0, z_top - 0.004), (0.05, 0.05, 0.008), 0.003), m), 'chrome', var=0.6)
    k.add(xf(lathe_axis('cst_swivel', [(0.0, z_top - 0.026), (0.018, z_top - 0.024), (0.02, z_top - 0.012), (0.012, z_top - 0.008), (0.0, z_top - 0.008)], segments=sw_seg), m),
          'chrome', var=0.7)
    for s in (-1, 1):
        k.add(xf(bxc('cst_fork', (0.012, s * 0.014, z_top - 0.026 - wheel_r * 0.6), (0.03, 0.004, wheel_r * 1.3), 0.0015), m), 'chrome', var=0.7)
    k.add(xf(lathe_axis('cst_wheel', [(0.0, -0.011), (wheel_r * 0.8, -0.011), (wheel_r, -0.007), (wheel_r, 0.007), (wheel_r * 0.8, 0.011), (0.0, 0.011)],
                        M(0.02, 0.0, wheel_r) @ Matrix.Rotation(math.radians(90), 4, 'X'), wh_seg), m), 'rubber', 0x1c1c1c)
    k.add(xf(rod('cst_axle', (0.02, -0.018, wheel_r), (0.02, 0.018, wheel_r), 0.004, ax_seg), m), 'chrome')


def glide(k, x, y, z_top, r=0.016):
    k.add(xf(lathe_axis('glide', [(0.0, 0.0), (r, 0.0), (r * 1.05, 0.006), (r * 0.7, 0.012), (r * 0.5, z_top), (0.0, z_top)], segments=20), M(x, y, 0.0)),
          'plastic', 0x1a1a1a)


def rack_round():
    """SM_NS_Thrift_RackRound: a chrome round garment rack, 1.2 m across."""
    k = Kit('thr', 'SM_NS_Thrift_RackRound', TEXTURE_SIZES, dust=0.2, low_grime=0.08)
    # (sides and segments sized for the spec's 6k: the ring has a vertex at each of the 62 hanger points)
    ring = [(RR_R * math.cos(2 * math.pi * i / 62), RR_R * math.sin(2 * math.pi * i / 62), RR_Z) for i in range(62)]
    k.add(tube('ring', ring, RR_T, 10, closed=True), 'chrome', var=0.15)
    for a in (45, 135, 225, 315):
        ca, sa = math.cos(math.radians(a)), math.sin(math.radians(a))
        k.add(tube('arm', bend([(ca * 0.03, sa * 0.03, RR_Z + 0.07), (ca * 0.25, sa * 0.25, RR_Z + 0.07), (ca * (RR_R - 0.04), sa * (RR_R - 0.04), RR_Z),
                                (ca * (RR_R - RR_T), sa * (RR_R - RR_T), RR_Z)], 0.05), 0.0095, 8), 'chrome', var=0.2)
        k.add(xf(lathe_axis('arm_clamp', [(0.0, -0.018), (0.02, -0.018), (0.021, 0.018), (0.0, 0.018)], segments=12),
                 M(ca * (RR_R - 0.02), sa * (RR_R - 0.02), RR_Z, a, 90)), 'chrome', var=0.3)
    k.add(lathe_axis('hub', [(0.0, RR_Z + 0.045), (0.034, RR_Z + 0.045), (0.036, RR_Z + 0.06), (0.034, RR_Z + 0.095), (0.026, RR_Z + 0.105), (0.0, RR_Z + 0.106)], segments=16),
          'chrome', var=0.2)
    # telescoping post: the outer tube, its collar and T-knob, the inner tube up to the hub
    k.add(rod('post_out', (0, 0, 0.07), (0, 0, 1.06), 0.0175, 16), 'chrome', var=0.3)
    k.add(lathe_axis('collar', [(0.0, 1.05), (0.021, 1.05), (0.022, 1.09), (0.0175, 1.095), (0.0, 1.095)], segments=16), 'chrome', var=0.4)
    k.add(rod('post_in', (0, 0, 1.09), (0, 0, RR_Z + 0.05), 0.0145, 16), 'chrome', var=0.1)
    k.add(rod('knob_screw', (0.02, 0, 1.072), (0.055, 0, 1.072), 0.005, 6), 'steel')
    k.add(xf(lathe_axis('knob', [(0.0, 0.0), (0.012, 0.0), (0.016, 0.006), (0.016, 0.02), (0.01, 0.026), (0.0, 0.026)], segments=10),
             M(0.055, 0.0, 1.072, 0, 90)), 'plastic', 0x111111)
    # cross base: four legs of 1 in square tube, a hub plate, a caster at each end
    k.add(lathe_axis('base_hub', [(0.0, 0.05), (0.05, 0.05), (0.052, 0.075), (0.03, 0.09), (0.0, 0.09)], segments=16), 'chrome', var=0.6)
    for a in (0, 90, 180, 270):
        m = M(0, 0, 0, a)
        k.add(xf(bxc('leg', (0.24, 0.0, 0.0725), (0.42, 0.0254, 0.0254), 0.004, 1), m), 'chrome', var=0.7)
        k.add(xf(bxc('leg_cap', (0.452, 0.0, 0.0725), (0.006, 0.026, 0.026), 0.002), m), 'plastic', 0x111111)
        ca, sa = math.cos(math.radians(a)), math.sin(math.radians(a))
        caster(k, ca * 0.425, sa * 0.425, 0.06, a, lite=True)
    k.grime += [((0.0, 0.0, 1.07), 0.08, 0.5)]
    k.polish += [((0.0, 0.0, 1.07), 0.05, 0.5)]
    return [k.finish()]


def rack_straight():
    """SM_NS_Thrift_RackStraight: a chrome two-way rack, two 1.8 m rails on H-frame ends with casters."""
    k = Kit('thk', 'SM_NS_Thrift_RackStraight', TEXTURE_SIZES, dust=0.2, low_grime=0.08)
    for xs in (-SR_X, SR_X):
        k.add(rod('rail', (xs, -SR_HALF - 0.02, SR_Z), (xs, SR_HALF + 0.02, SR_Z), SR_T, 16), 'chrome', dir=1, var=0.15)
        for ys in (-SR_HALF - 0.02, SR_HALF + 0.02):
            k.add(xf(lathe_axis('rail_cap', [(0.0, 0.0), (SR_T + 0.002, 0.0), (SR_T + 0.002, 0.008), (SR_T * 0.6, 0.012), (0.0, 0.012)], segments=16),
                     M(xs, ys, SR_Z) @ Matrix.Rotation(math.radians(90 if ys < 0 else -90), 4, 'X')), 'plastic', 0x111111)
    for ys in (-SR_HALF + 0.05, SR_HALF - 0.05):
        for xs in (-SR_X, SR_X):
            k.add(rod('upright', (xs, ys, 0.085), (xs, ys, SR_Z - 0.02), 0.0158, 20), 'chrome', var=0.3)
            k.add(xf(lathe_axis('tee', [(0.0, -0.035), (0.021, -0.035), (0.022, 0.035), (0.0, 0.035)], segments=16),
                     M(xs, ys, SR_Z, 0, 0) @ Matrix.Rotation(math.radians(90), 4, 'X')), 'chrome', var=0.3)
        # the foot bar (square tube across), a cross tie at mid height
        k.add(bxc('foot', (0.0, ys, 0.072), (2 * SR_X + 0.12, 0.0318, 0.0318), 0.004), 'chrome', var=0.7)
        for xs in (-SR_X - 0.06, SR_X + 0.06):
            k.add(bxc('foot_cap', (xs + (0.003 if xs > 0 else -0.003), ys, 0.072), (0.006, 0.033, 0.033), 0.002), 'plastic', 0x111111)
            caster(k, xs - (0.02 if xs > 0 else -0.02), ys, 0.056, 0)
        k.add(rod('tie', (-SR_X, ys, 0.9), (SR_X, ys, 0.9), 0.011, 14), 'chrome', var=0.3)
    # the long stretcher between the feet
    k.add(bxc('stretcher', (0.0, 0.0, 0.072), (0.0318, 2 * SR_HALF - 0.1, 0.0318), 0.004), 'chrome', dir=1, var=0.6)
    return [k.finish()]


# ------------------------------------------------------------------ garment stand-ins (until looks' rack statics land)

def wire_hanger(kind='wire'):
    """A wire coat hanger in the YZ plane, its hook's inside top at the origin (where it rests on a rail)."""
    rh = 0.013
    hook = [(0.0, -rh * math.sin(math.radians(a)), -rh + rh * math.cos(math.radians(a))) for a in np.linspace(-160.0, 110.0, 18)]
    neck = [(0.0, 0.004, -0.045), (0.0, 0.0, -0.062), (0.0, 0.004, -0.07), (0.0, -0.002, -0.078), (0.0, 0.0, -0.082)]
    body = [(0.0, 0.205, -0.205), (0.0, 0.212, -0.214), (0.0, 0.2, -0.218), (0.0, -0.2, -0.218), (0.0, -0.212, -0.214), (0.0, -0.205, -0.205),
            (0.0, -0.004, -0.084)]
    pts = hook + neck + body
    return tube('hanger', bend(pts, 0.008, 4), 0.0017 if kind == 'wire' else 0.004, 6)


def superellipse(w, d, n=28, p=0.55):
    """Ring points (x, y) of a rounded rectangle-ish section: half depth d along X, half width w along Y."""
    out = []
    for i in range(n):
        t = 2 * math.pi * i / n
        c, s = math.cos(t), math.sin(t)
        out.append((d * math.copysign(abs(c) ** p, c), w * math.copysign(abs(s) ** p, s)))
    return out


def resample(rows, step):
    """Rows of numbers (the first column a running coordinate, increasing or decreasing) resampled every `step` of it."""
    out = [rows[0]]
    for a, b in zip(rows, rows[1:]):
        n = max(1, int(math.ceil(abs(b[0] - a[0]) / step)))
        for k in range(1, n + 1):
            t = k / n
            out.append(tuple(a[i] + (b[i] - a[i]) * t for i in range(len(a))))
    return out


def loft_z(name, profile, n=48, p=0.55, yc=0.0, xc=0.0, step=0.025):
    """A closed body lofted through horizontal sections: profile [(z, half width, half depth)] top to bottom."""
    rings = []
    for z, w, d in resample(profile, step):
        rings.append([(xc + x, yc + y, z) for x, y in superellipse(w, d, n, p)])
    return core.loft(name, rings, caps=True)


def loft_path(name, path, widths, depth, n=32, p=0.6, step=0.03):
    """A flattened tube along a path in the YZ plane (a sleeve, a pant leg): widths[i] its half width across the path
    (in YZ), depth (or depths) its half thickness along X."""
    depths = depth if isinstance(depth, (list, tuple)) else [depth] * len(path)
    acc = [0.0]
    for a, b in zip(path, path[1:]):
        acc.append(acc[-1] + (Vector(b) - Vector(a)).length)
    rows = resample([(acc[i], *path[i], widths[i], depths[i]) for i in range(len(path))], step)
    pts = [Vector(r[1:4]) for r in rows]
    widths = [r[4] for r in rows]
    depth = [r[5] for r in rows]
    rings = []
    for i, q in enumerate(pts):
        t = (pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)]).normalized()
        across = t.cross(Vector((1.0, 0.0, 0.0))).normalized()
        dd = depth[i] if isinstance(depth, (list, tuple)) else depth
        ring = []
        for x, y in superellipse(widths[i], dd, n, p):
            ring.append(q + Vector((x, 0.0, 0.0)) + across * y)
        rings.append(ring)
    return core.loft(name, rings, caps=True)


def drape(obj, seed, amp=0.008, period=0.085, top=-0.1, bottom=-0.8, sway=0.0):
    """Folds that hang from the shoulders: vertical ripples in X growing toward the hem, a little noise, a sway."""
    me = obj.data
    P = np.array([v.co[:] for v in me.vertices])
    t = np.clip((top - P[:, 2]) / max(top - bottom, 1e-6), 0.0, 1.0)
    n1 = texmaps.value_noise(P * np.array([1.0, 1.0, 0.25]) + seed, 0.12, seed=seed, octaves=3) - 0.5
    ph = P[:, 1] / period * 2 * math.pi + n1 * 3.0
    n2 = texmaps.value_noise(P * np.array([1.0, 1.0, 0.4]) + seed * 0.37, 0.05, seed=seed + 3, octaves=2) - 0.5
    P[:, 0] += (np.sin(ph) * amp * (0.25 + 0.75 * t)) + n1 * amp * 1.2 * t + n2 * amp * 0.5
    # the hem hangs unevenly
    hem = np.clip((P[:, 2] - bottom - 0.05) / -0.05, 0.0, 1.0)
    P[:, 2] += n1 * 0.02 * hem
    # hems flare a touch; the cloth swings off the vertical
    P[:, 1] *= 1.0 + 0.04 * t * t
    P[:, 0] += sway * t * t
    for v, q in zip(me.vertices, P):
        v.co = q
    me.shade_smooth()


def hung(kind):
    """SM_NS_Thrift_Hung_<Kind>: a stand-in garment on a wire hanger (spec 4.7's SM_NS_Rack_<Kind> frame: origin at the
    hanger hook where it rests on the rail, front toward -X, shoulders along Y). Slots: 0 cloth [tint], 1 hanger."""
    name = f'SM_NS_Thrift_Hung_{kind.capitalize()}'
    k = Kit(f'thp_{kind}', name, TEXTURE_SIZES, dust=0.0, low_grime=0.0, groups=['cloth', 'hanger'])
    k.cavity_amt = 0.45
    k.cavity = 0.03         # the folds are centimeters deep: their hollows shade
    rnd = random.Random(len(kind) * 17)
    parts_ = []
    if kind == 'tee':
        body = loft_z('tee_body', [(-0.084, 0.045, 0.008), (-0.10, 0.10, 0.013), (-0.14, 0.16, 0.016), (-0.19, 0.205, 0.018), (-0.23, 0.218, 0.02),
                                   (-0.33, 0.224, 0.021), (-0.48, 0.23, 0.021), (-0.62, 0.238, 0.02), (-0.74, 0.245, 0.018), (-0.785, 0.247, 0.012),
                                   (-0.79, 0.24, 0.004)])
        parts_.append(body)
        for s in (-1, 1):
            parts_.append(loft_path('tee_sleeve', [(0.0, s * 0.15, -0.17), (0.0, s * 0.215, -0.235), (0.0, s * 0.265, -0.30), (0.0, s * 0.295, -0.345),
                                                   (0.0, s * 0.30, -0.36)],
                                    [0.06, 0.066, 0.066, 0.06, 0.055], [0.012, 0.014, 0.014, 0.012, 0.007]))
        parts_.append(tube('tee_collar', [(0.016 * math.cos(a), 0.058 * math.sin(a), -0.09) for a in np.linspace(0, 2 * math.pi, 28, endpoint=False)],
                           0.0045, 8, closed=True))
        sway, amp = 0.0, 0.007
    elif kind == 'hoodie':
        body = loft_z('hd_body', [(-0.084, 0.05, 0.02), (-0.10, 0.12, 0.032), (-0.15, 0.20, 0.04), (-0.205, 0.245, 0.045), (-0.26, 0.255, 0.048),
                                  (-0.40, 0.258, 0.05), (-0.55, 0.26, 0.05), (-0.64, 0.255, 0.048), (-0.66, 0.235, 0.04), (-0.70, 0.232, 0.035),
                                  (-0.705, 0.22, 0.012)])
        parts_.append(body)
        for s in (-1, 1):
            parts_.append(loft_path('hd_sleeve', [(0.01, s * 0.17, -0.16), (0.014, s * 0.235, -0.23), (0.02, s * 0.27, -0.32), (0.026, s * 0.283, -0.45),
                                                  (0.024, s * 0.272, -0.60), (0.02, s * 0.264, -0.66), (0.02, s * 0.262, -0.70)],
                                    [0.07, 0.075, 0.07, 0.062, 0.055, 0.044, 0.04], [0.026, 0.03, 0.03, 0.028, 0.026, 0.024, 0.016]))
        parts_.append(blob('hd_hood', (0.07, 0.0, -0.13), (0.12, 0.26, 0.2), seed=3, detail=0.08, subdiv=3, wrinkle=0.15, smooth=2))
        parts_.append(soft_slab('hd_pocket', 0.014, 0.30, 0.16, 0.006, 2, 10, 6, matrix=M(-0.05, 0.0, -0.69)))
        for s in (-1, 1):
            parts_.append(rod('hd_cord', (-0.05, s * 0.035, -0.10), (-0.052, s * 0.04, -0.30 - 0.03 * s), 0.0028, 8))
        sway, amp = 0.0, 0.01
    elif kind == 'jacket':
        body = loft_z('jk_body', [(-0.084, 0.06, 0.028), (-0.10, 0.13, 0.04), (-0.15, 0.215, 0.05), (-0.20, 0.25, 0.055), (-0.25, 0.258, 0.058),
                                  (-0.40, 0.255, 0.058), (-0.55, 0.25, 0.056), (-0.62, 0.244, 0.052), (-0.64, 0.226, 0.046), (-0.685, 0.224, 0.04),
                                  (-0.69, 0.214, 0.012)], p=0.5)
        parts_.append(body)
        for s in (-1, 1):
            parts_.append(loft_path('jk_sleeve', [(0.01, s * 0.19, -0.16), (0.014, s * 0.25, -0.22), (0.018, s * 0.285, -0.31), (0.024, s * 0.295, -0.45),
                                                  (0.022, s * 0.285, -0.60), (0.02, s * 0.277, -0.65), (0.02, s * 0.275, -0.69)],
                                    [0.075, 0.08, 0.075, 0.066, 0.058, 0.05, 0.046], [0.03, 0.034, 0.034, 0.032, 0.03, 0.028, 0.018]))
        # a stand collar and the zip's tape down the front
        parts_.append(core.sweep('jk_collar', [(0.034 * math.cos(a), -0.07 * math.sin(a), -0.108 + 0.012 * math.cos(a)) for a in np.linspace(-math.pi * 0.92, math.pi * 0.92, 28)],
                                 [(0.0, 0.006), (0.036, 0.0075), (0.036, -0.0045), (0.0, -0.006)]))
        parts_.append(bxc('jk_zip', (-0.059, 0.0, -0.39), (0.004, 0.012, 0.56), 0.002))
        sway, amp = 0.0, 0.006
    else:   # pants folded over the bar
        parts_.append(soft_slab('pt_front', 0.022, 0.36, 0.52, 0.01, 2, 10, 12, noise_amt=0.0,
                                matrix=M(-0.022, 0.0, -0.215 - 0.52)))
        parts_.append(soft_slab('pt_back', 0.02, 0.34, 0.46, 0.01, 2, 10, 12, matrix=M(0.022, 0.0, -0.215 - 0.46)))
        parts_.append(xf(core.lathe('pt_fold', [(0.03, -0.18), (0.031, 0.18)], 24), Matrix.Translation((0.0, 0.0, -0.205)) @ Matrix.Rotation(math.radians(90), 4, 'X')))
        sway, amp = 0.0, 0.004
    cloth = core.join(parts_, 'cloth') if len(parts_) > 1 else parts_[0]
    drape(cloth, rnd.randint(1, 99), amp=amp * 2.3, top=-0.1, bottom=float(min(v.co.z for v in cloth.data.vertices)), sway=sway)
    k.add(cloth, 'cloth', CLOTH_GREY, group='cloth')
    k.add(wire_hanger(), 'chrome', 0xffffff, group='hanger', var=0.5)
    return [k.finish(angle=60.0)]


def folded(kind):
    """SM_NS_Thrift_Folded_<Kind>: a stack of 3 folded garments (spec 4.7's SM_NS_Folded_<Kind> frame: origin at the
    bottom middle, front toward -X). Slot 0 cloth [tint]."""
    name = f'SM_NS_Thrift_Folded_{kind.capitalize()}'
    k = Kit(f'thq_{kind}', name, TEXTURE_SIZES, dust=0.0, low_grime=0.0, groups=['cloth'])
    k.cavity_amt = 0.4
    k.cavity = 0.01
    w, d, h = {'tee': (0.30, 0.235, 0.028), 'jeans': (0.33, 0.26, 0.048), 'hoodie': (0.34, 0.275, 0.06)}[kind]
    rnd = random.Random(sum(map(ord, kind)))
    z = 0.0
    parts_ = []
    for i in range(3):
        hh = h * rnd.uniform(0.92, 1.08)
        dd, ww = d * rnd.uniform(0.97, 1.03), w * rnd.uniform(0.97, 1.03)
        mat = M(rnd.uniform(-0.012, 0.012), rnd.uniform(-0.014, 0.014), z, rnd.uniform(-4, 4) + (3.0 if i == 2 else 0.0))
        parts_ += folded_layer(kind, dd, ww, hh, rnd.randint(1, 99), mat)
        z += hh * 0.9
    cloth = core.join(parts_, 'cloth')
    k.add(cloth, 'cloth', CLOTH_GREY, group='cloth')
    return [k.finish(angle=60.0)]


def folded_layer(kind, d, w, h, seed, mat):
    """One folded garment as a shop folds it (front toward -X, origin at its bottom middle): a slab whose front edge is
    the bottom fold (a full round), its sides the sleeves or legs folded under (rounded, a little fuller), its top
    creased; then what shows on top: a tee's collar and neckline at the back edge, a hoodie's kangaroo pocket, folded
    hood and cords, the jeans' waistband with its loops, a back pocket and the fly's stitching."""
    rnd = random.Random(seed)
    out = []
    g = soft_slab('fold', d, w, h, h * 0.47, 9, 9, 9, sag=0.0, seed=seed, noise_amt=0.0)
    me = g.data
    P = np.array([v.co[:] for v in me.vertices])
    top = np.clip((P[:, 2] - h * 0.5) / (h * 0.5), 0.0, 1.0)
    # the top's soft creases: shallow ridges across (from the fold) and a few long diagonal ones, and a slight dome
    n1 = texmaps.value_noise(P * np.array([1.0, 0.35, 1.0]) + seed, 0.05, seed=seed, octaves=2) - 0.5
    n2 = texmaps.value_noise(P + seed * 0.71, 0.025, seed=seed + 9, octaves=2) - 0.5
    rr = (P[:, 0] / (d / 2)) ** 2 + (P[:, 1] / (w / 2)) ** 2
    P[:, 2] += top * (h * 0.06 * np.clip(1.0 - rr, 0.0, 1.0) + 0.0025 * n1 + 0.0012 * n2)
    # the back edge (the shoulders and collar on a tee, the waistband on jeans) sits a touch higher than the fold
    P[:, 2] += top * h * 0.08 * np.clip(P[:, 0] / (d / 2), 0.0, 1.0)
    # the sides bulge out a little where the sleeves are folded under
    side = np.clip((np.abs(P[:, 1]) - (w / 2 - 0.04)) / 0.04, 0.0, 1.0)
    P[:, 1] += np.sign(P[:, 1]) * side * 0.004 * (1.0 - np.abs(P[:, 2] - h / 2) / (h / 2))
    for v, p in zip(me.vertices, P):
        v.co = p
    out.append(g)
    zt = h * 1.06          # about the top surface
    xb = d / 2             # the back edge
    if kind == 'tee':
        # the rib collar: an oval at the back edge, its back half rolled over the edge, the neckline dipping forward
        ring = []
        for a in np.linspace(0, 2 * math.pi, 40, endpoint=False):
            x = xb - 0.035 - 0.032 * math.cos(a)
            ring.append((x, 0.072 * math.sin(a), zt - 0.0015 - 0.012 * min(max((x - (xb - 0.015)) / 0.015, 0.0), 1.0)))   # rolls down the edge
        out.append(tube('collar', ring, 0.0045, 6, closed=True))
        # the back of the neck showing inside the collar (a slightly lower patch) and the shoulder seams
        out.append(soft_slab('neck_in', 0.05, 0.12, 0.003, 0.0015, 3, 3, 3, matrix=M(xb - 0.04, 0.0, zt - 0.004)))
        for s_ in (-1, 1):
            out.append(tube('shoulder', [(xb - 0.004, s_ * 0.075, zt - 0.002), (xb - 0.018, s_ * (w / 2 - 0.03), zt - 0.003)], 0.0012, 6))
    elif kind == 'jeans':
        # the waistband along the back edge with its belt loops and the button, a back pocket, the yoke seam
        out.append(soft_slab('band', 0.038, w * 0.96, 0.006, 0.0028, 5, 5, 5, matrix=M(xb - 0.024, 0.0, zt - 0.004)))
        for yy in (-0.12, -0.04, 0.05, 0.13):
            out.append(bxc('loop', (xb - 0.024, yy + rnd.uniform(-0.01, 0.01), zt + 0.0035), (0.04, 0.011, 0.003), 0.0013))
        out.append(xf(lathe_axis('button', [(0.0, 0.0), (0.008, 0.0), (0.0085, 0.002), (0.004, 0.0035), (0.0, 0.004)], segments=16),
                      M(xb - 0.024, w / 2 - 0.03, zt + 0.002)))
        pk = [(0.0, -0.065), (0.11, -0.07), (0.135, 0.0), (0.11, 0.07), (0.0, 0.065)]     # a pentagonal patch pocket
        out.append(xf(parts.panel('pocket', pk, 0.0, 0.003, 0.0012), M(xb - 0.19, -0.05, zt - 0.002)))
        out.append(tube('yoke', [(xb - 0.05, -w / 2 + 0.02, zt - 0.001), (xb - 0.07, 0.0, zt - 0.0005), (xb - 0.05, w / 2 - 0.02, zt - 0.001)], 0.0016, 6))
    else:   # hoodie
        # the hood folded back over the shoulders: a soft roll along the back edge
        hood = soft_slab('hood', 0.07, 0.24, 0.022, 0.01, 6, 6, 6, sag=0.0, seed=seed + 1, noise_amt=0.004, matrix=M(xb - 0.05, 0.0, zt - 0.006))
        out.append(hood)
        # the kangaroo pocket toward the fold, its two openings angled, the cords lying across
        kp = [(-0.06, -0.12), (0.03, -0.09), (0.05, -0.06), (0.05, 0.06), (0.03, 0.09), (-0.06, 0.12)]
        out.append(xf(parts.panel('kpocket', kp, 0.0, 0.004, 0.0018), M(-d / 2 + 0.11, 0.0, zt - 0.003)))
        for s_ in (-1, 1):
            cord = [(xb - 0.06, s_ * 0.03, zt + 0.012), (xb - 0.10, s_ * 0.045, zt + 0.003), (xb - 0.15, s_ * (0.035 + 0.02 * rnd.random()), zt + 0.002)]
            out.append(tube('cord', bend(cord, 0.02, 4), 0.003, 8))
            out.append(rod('aglet', cord[-1], (cord[-1][0] - 0.02, cord[-1][1], cord[-1][2]), 0.0033, 8))
        # the rib hem along the fold
        out.append(soft_slab('rib', 0.02, w * 0.9, 0.006, 0.0028, 4, 4, 4, matrix=M(-d / 2 + 0.02, 0.0, zt - 0.004)))
    for o in out:
        o.data.transform(mat)
        o.data.shade_smooth()
    return out


# ------------------------------------------------------------------ the folding table

TB_L, TB_W, TB_H = 1.83, 0.76, 0.74
TABLE_STACKS = [(xs, ys, TB_H) for xs in (-0.19, 0.19) for ys in (-0.66, -0.22, 0.22, 0.66)]


def draw_refold(s, w, h):
    s.rect(0, 0, w, h, 0xf3f0e6, rough=0.8)
    for i in range(5):
        s.rect(0, h - 26 - i * 14, w, 0.6, 0x9fb7d8, rough=0.8)
    s.rect(14, 0, 0.6, h, 0xe4a0a0, rough=0.8)
    hand_mid(s, 'PLEASE REFOLD', w / 2 + 4, h * 0.64, 20, w - 24, 0x1a1a1a, width=0.1, seed=4)
    hand_mid(s, 'Thank you!  - Bea', w / 2 + 4, h * 0.28, 13, w - 30, 0x1b2f7a, width=0.08, seed=9)


def draw_tape(s, w, h):
    s.rect(0, 0, w, h, 0xd9cfae, rough=0.6, alpha=0.75)
    s.rect(0, 0, 1.0, h, 0xc8bd98, rough=0.6, alpha=0.6)
    s.rect(w - 1.0, 0, 1.0, h, 0xc8bd98, rough=0.6, alpha=0.6)


def table():
    """SM_NS_Thrift_Table: a church-basement folding table (6 ft): woodgrain laminate top with a brown T-molding, steel
    apron, folding tube legs with braces and glides; a PLEASE REFOLD card taped on."""
    k = Kit('tht', 'SM_NS_Thrift_Table', TEXTURE_SIZES, dust=0.15, low_grime=0.1)
    top = parts.panel('top', core.rounded_rect(-TB_W / 2, -TB_L / 2, TB_W / 2, TB_L / 2, 0.03, 4), TB_H - 0.024, TB_H, 0.003, 2)
    k.add(top, 'laminate', 0x8f6a45, dir=1, var=0.2)
    # the T-molding round the top's edge: a symmetric profile about a path just outside the edge, so it stands proud of
    # the laminate whichever way the sweep turns its frame
    edge = core.sweep('tmold', [(x, y, TB_H - 0.012) for x, y in core.rounded_rect(-TB_W / 2 - 0.002, -TB_L / 2 - 0.002, TB_W / 2 + 0.002, TB_L / 2 + 0.002, 0.032, 4)],
                      [(0.005, 0.0132), (-0.005, 0.0132), (-0.005, -0.0132), (0.005, -0.0132)], closed=True)
    k.add(edge, 'vinyl', 0x3a2a1e)
    for xs in (-1, 1):
        k.add(bxc('apron', (xs * (TB_W / 2 - 0.05), 0.0, TB_H - 0.045), (0.025, TB_L - 0.1, 0.04), 0.002), 'powder', 0x4b4033, dir=1)
    for ys in (-1, 1):
        k.add(bxc('apron_end', (0.0, ys * (TB_L / 2 - 0.05), TB_H - 0.045), (TB_W - 0.1, 0.025, 0.04), 0.002), 'powder', 0x4b4033)
        # the folding leg: two uprights, the bottom bar, the brace to the center rail
        y0 = ys * (TB_L / 2 - 0.13)
        for xs in (-1, 1):
            k.add(rod('leg', (xs * 0.30, y0, 0.03), (xs * 0.30, y0, TB_H - 0.03), 0.0127, 16), 'powder', 0x4b4033, dir=2)
            glide(k, xs * 0.30, y0, 0.03, 0.014)
            k.add(rod('brace', (xs * 0.30, y0, 0.42), (xs * 0.06, y0 - ys * 0.38, TB_H - 0.06), 0.008, 10), 'powder', 0x4b4033)
        k.add(rod('legbar', (-0.30, y0, 0.14), (0.30, y0, 0.14), 0.011, 14), 'powder', 0x4b4033)
        k.add(rod('legtop', (-0.32, y0, TB_H - 0.045), (0.32, y0, TB_H - 0.045), 0.011, 14), 'powder', 0x4b4033)
    k.add(bxc('center_rail', (0.0, 0.0, TB_H - 0.06), (0.04, TB_L * 0.55, 0.02), 0.002), 'powder', 0x4b4033, dir=1)
    k.panel('refold', 0.127, 0.076, draw_refold)
    k.panel('tape', 0.05, 0.019, draw_tape)
    c = Vector((-TB_W / 2 + 0.07, 0.42, TB_H + 0.0006))
    k.decal('refold', c, u=(0, -1, 0), v=(1, 0, 0), lift=0.0)
    k.decal('tape', c + Vector((0.03, -0.055, 0.0002)), u=(0, -1, 0), v=(1, 0, 0), w=0.019, h=0.05, lift=0.0)
    k.decal('tape', c + Vector((0.03, 0.055, 0.0002)), u=(0, -1, 0), v=(1, 0, 0), w=0.019, h=0.05, lift=0.0)
    k.polish += [((-TB_W / 2, 0.0, TB_H), 0.5, 0.4)]
    return [k.finish()]


# ------------------------------------------------------------------ the storefront (SITE)

BRONZE = 0x2b2622
SIGN_GROUND = 0x1f4a44      # the sign band's painted ground (a deep teal-green)
SIGN_INK = 0xefe4c8         # its cream letters
SIGN_RED = 0xb0432e


def seg_box(name, p1, p2, off, thick, z0, z1, r=0.002, ext=0.0):
    """A box along the floor-plan segment p1 -> p2 (x, y), its middle offset `off` to the segment's left, `thick` across,
    from z0 to z1; ext lengthens it at both ends."""
    a, b = Vector((p1[0], p1[1], 0.0)), Vector((p2[0], p2[1], 0.0))
    d = b - a
    L = d.length
    yaw = math.degrees(math.atan2(d.y, d.x))
    n = Vector((-d.y, d.x, 0.0)).normalized()
    c = (a + b) / 2 + n * off
    return xf(parts.rbox(name, (0.0, 0.0, 0.0), (L + 2 * ext, thick, z1 - z0), r, 2), M(c.x, c.y, (z0 + z1) / 2, yaw))


def seg_quad(name, p1, p2, z0, z1, toward):
    """A vertical quad over the floor-plan segment p1 -> p2 from z0 to z1, facing the side `toward` (a point) is on;
    UV 0..1 (u along p1 -> p2 seen from that side)."""
    a = Vector((p1[0], p1[1], 0.0))
    b = Vector((p2[0], p2[1], 0.0))
    d = (b - a)
    n = Vector((-d.y, d.x, 0.0))
    t = Vector((toward[0], toward[1], 0.0)) - a
    if n.dot(t) < 0:
        a, b = b, a
    return quad(name, [(a.x, a.y, z0), (b.x, b.y, z0), (b.x, b.y, z1), (a.x, a.y, z1)])


def draw_sign(s, w, h):
    """The sign band: hand-painted letters on the teal ground, weathered (12.10 x 0.64 m, in mm)."""
    rnd = random.Random(22)
    s.rect(0, 0, w, h, SIGN_GROUND, rough=0.62)
    # brush streaks and sun fade in the ground
    for i in range(70):
        y = rnd.uniform(0, h)
        s.rect(rnd.uniform(-200, w), y, rnd.uniform(600, 3000), rnd.uniform(4, 14), rnd.choice((0x245650, 0x1b403b, 0x2a5a52)), rough=0.62,
               alpha=rnd.uniform(0.15, 0.4))
    s.rect(0, h * 0.62, w, h * 0.38, 0x3a6a62, rough=0.66, alpha=0.18)
    # a cream pinstripe border, the red rules
    s.rect(22, 22, w - 44, 7, SIGN_INK, rough=0.55)
    s.rect(22, h - 29, w - 44, 7, SIGN_INK, rough=0.55)
    s.rect(22, 22, 7, h - 44, SIGN_INK, rough=0.55)
    s.rect(w - 29, 22, 7, h - 44, SIGN_INK, rough=0.55)
    title = 'SECOND CHANCE THRIFT'
    cap = 300.0
    size = cap / 0.72
    tw = text_w(s.s, title, 'Black', 1.08) * size
    ty = h * 0.40
    # a drop shadow in the dark green, then the cream letters
    s.text(title, w / 2 + 9, ty - 9, size, 0x0f2622, 'Black', align='CENTER', tracking=1.08, rough=0.6)
    s.text(title, w / 2, ty, size, SIGN_INK, 'Black', align='CENTER', tracking=1.08, rough=0.5)
    # the script line under it, painted in red with a cream edge
    sub = 'St. Agnes Outreach'
    hs = 120.0
    hw_ = hand_w(sub, hs)
    hand(s, sub, w / 2 - hw_ / 2 + 3, 46 - 3, hs, 0x0f2622, width=0.11, slant=0.18, seed=5, smooth=6, wobble=0.004)
    hand(s, sub, w / 2 - hw_ / 2, 46, hs, SIGN_RED, width=0.1, slant=0.18, seed=5, smooth=6, wobble=0.004)
    # the rules either side of the title, the small lines at each end (what's really sold)
    for side in (-1, 1):
        x0 = w / 2 + side * (tw / 2 + 120)
        x1 = w / 2 + side * (w / 2 - 260)
        s.rect(min(x0, x1), ty + cap * 0.46, abs(x1 - x0), 10, SIGN_RED, rough=0.55)
        s.rect(min(x0, x1), ty + cap * 0.46 - 26, abs(x1 - x0), 5, SIGN_INK, rough=0.55)
    s.mid('CLOTHING  ·  SHOES', 260 + 1250, h * 0.30, 72, 2400, SIGN_INK, 'Bold', tracking=1.3)
    s.mid('HATS  ·  GLASSES', w - 260 - 1250, h * 0.30, 72, 2400, SIGN_INK, 'Bold', tracking=1.3)
    # weather: peeling to the grey primer and the wood, flaking letters, rust runs from the fixings
    for i in range(46):
        cx, cy = rnd.uniform(40, w - 40), rnd.uniform(20, h - 20)
        r = rnd.uniform(6, 40)
        pts = [(cx + r * rnd.uniform(0.5, 1.3) * math.cos(a), cy + r * rnd.uniform(0.4, 1.1) * math.sin(a))
               for a in np.linspace(0, 2 * math.pi, 9, endpoint=False)]
        s.poly(pts, rnd.choice((0x8a8678, 0x9b8a6a, 0x6e6a5c)), rough=0.8, alpha=rnd.uniform(0.6, 0.95))
    for fx in np.linspace(300, w - 300, 9):
        s.circle(fx, h / 2, 9, 0x4a4036, metal=0.6, rough=0.5)
        s.rect(fx - 4, h / 2 - rnd.uniform(60, 200), 8, rnd.uniform(60, 200), 0x6a4a2e, rough=0.7, alpha=0.4)
    for i in range(120):
        x = rnd.uniform(0, w)
        s.rect(x, 0, rnd.uniform(1.5, 5), rnd.uniform(40, h * 0.9), 0x101e1b, rough=0.7, alpha=rnd.uniform(0.08, 0.22))


def draw_poster_yellow(s, w, h):
    s.rect(0, 0, w, h, 0xf3d23a, rough=0.75)
    s.rect(14, 14, w - 28, h - 28, 0xf6da50, rough=0.75)
    s.mid('YELLOW', w / 2, h * 0.80, 92, w - 50, 0x16120c, 'Black', tracking=1.02)
    s.mid('TAGS', w / 2, h * 0.64, 92, w - 50, 0x16120c, 'Black', tracking=1.02)
    s.circle(w / 2, h * 0.395, 92, 0xc8321e, rough=0.6)
    s.mid('50%', w / 2, h * 0.42, 66, 150, 0xffffff, 'Black')
    s.mid('OFF', w / 2, h * 0.335, 30, 120, 0xffffff, 'Black', tracking=1.2)
    s.mid('SATURDAYS', w / 2, h * 0.15, 52, w - 50, 0x16120c, 'Black', tracking=1.08)
    s.mid('ST. AGNES OUTREACH', w / 2, h * 0.055, 15, w - 80, 0x3a2a10, 'Bold', tracking=1.4)
    s.rect(0, 0, w, h, 0xffffff, rough=0.75, alpha=0.08)


def draw_poster_donate(s, w, h):
    s.rect(0, 0, w, h, 0xf4f1e8, rough=0.8)
    s.rect(0, h - 62, w, 62, 0x1f4a44, rough=0.7)
    s.mid('ST. AGNES OUTREACH', w / 2, h - 31, 26, w - 40, 0xefe4c8, 'Black', tracking=1.3)
    s.mid('DONATIONS', w / 2, h * 0.62, 70, w - 36, 0x1f4a44, 'Black')
    s.mid('IN BACK', w / 2, h * 0.47, 70, w - 36, 0x1f4a44, 'Black')
    hand_mid(s, 'ring the bell, God bless!', w / 2, h * 0.28, 30, w - 70, 0x1b2f7a, width=0.08, seed=8)
    s.mid('CLOTHING  ·  SHOES  ·  COATS', w / 2, h * 0.12, 18, w - 60, 0x3a3a3a, 'Bold', tracking=1.2)


def draw_paper_back(s, w, h):
    s.rect(0, 0, w, h, 0xe8e2cf, rough=0.85)
    s.rect(0, 0, w, h, 0x9a8a5a, rough=0.85, alpha=0.12)


def draw_number(s, w, h):
    s.rect(0, 0, w, h, 0xb8963e, metal=0.9, rough=0.3)
    s.rect(4, 4, w - 8, h - 8, 0x0c0c0c, rough=0.25)
    s.mid('220', w / 2, h / 2, h * 0.8, w * 0.95, 0xd8b45a, 'Black', metal=0.9, rough=0.3)


def draw_mosaic(path_name):
    """The recess floor's mosaic: HALLORAN'S (the five-and-dime that built the front) in oxblood on cream, a black
    border and a key pattern; drawn upside-down so it reads from the sidewalk (spec 0.3)."""
    w, h = 3160.0, 1200.0
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0xd9d0bc)
    s.rect(0, 0, w, 76, 0x1c1a18)
    s.rect(0, h - 76, w, 76, 0x1c1a18)
    for i in range(int(w / 101.6)):
        if i % 2 == 0:
            s.rect(i * 101.6, 76, 50.8, 25.4, 0x7a2a22)
            s.rect(i * 101.6, h - 101.6, 50.8, 25.4, 0x7a2a22)
    rot = math.pi
    cx, cy = w / 2, h / 2 + 30
    size = 330.0
    tw = text_w(s, "HALLORAN'S", 'Black', 1.12) * size
    s.text("HALLORAN'S", cx + tw / 2, cy + 0.36 * size, size, 0x6e2018, 'Black', align='LEFT', rot=rot, tracking=1.12)
    s.text('5 & 10', cx + text_w(s, '5 & 10', 'Black', 1.25) * 190 / 2, cy + 300 + 0.36 * 190, 190, 0x1c1a18, 'Black', rot=rot, tracking=1.25)
    return render_sheet(s, path_name, 1264, samples=8)


def front():
    """SM_NS_Thrift_Front + _Glass + _GlowWarm."""
    k = Kit('thf', 'SM_NS_Thrift_Front', TEXTURE_SIZES, dust=0.25, low_grime=0.35)
    k.cavity = 0.03
    k.params['mosaic'] = {'image': (draw_mosaic('thf_mosaic'), (28.90, 48.28, 32.10, 49.50)), 'tile': 0.0254}
    gl = []      # glass quads
    lamps = []   # glow parts
    gy = GLASS_Y + 0.02   # 49.40: the glass's plane
    wx0, wx1 = OX0 + 0.06, DX0 - RET      # the west display's glass run (24.46 .. 28.92)
    ex0, ex1 = DX1 + RET, OX1 - 0.06      # the east display's (32.08 .. 36.54)
    wret = ((wx1, gy), (DX0, DOOR_PLANE + 0.06))
    eret = ((ex0, gy), (DX1, DOOR_PLANE + 0.06))
    # --- the opening's end jambs (bronze box sections)
    for xa, xb in ((OX0, OX0 + 0.06), (OX1 - 0.06, OX1)):
        k.add(bx('jamb', xa, xb, gy - 0.10, FRONT_Y, 0.0, 3.32, 0.004), 'bronze', BRONZE, dir=2)
    # --- bulkheads under the displays: black glass panels on a bronze shoe, a sloped bronze sill
    for x0, x1 in ((wx0, wx1), (ex0, ex1)):
        n = max(2, int(round((x1 - x0) / 1.1)))
        pw = (x1 - x0) / n
        for i in range(n):
            a, b = x0 + i * pw + 0.004, x0 + (i + 1) * pw - 0.004
            replaced = (x0 < 30 and i == 1)
            panel = bx('bulk', a, b, gy + 0.045, gy + 0.06, 0.05, 0.45, 0.003)
            if replaced:
                k.add(panel, 'paint', 0x1d1f20)
            else:
                k.add(panel, 'blackglass', 0x0d0d0e, var=random.random())
        k.add(bx('shoe', x0, x1, gy + 0.04, gy + 0.09, 0.0, 0.05, 0.004), 'bronze', BRONZE)
        k.add(bx('bulk_back', x0, x1, gy - 0.02, gy + 0.045, 0.02, 0.45, 0.0), 'paint', 0x2a2724)
        sill = parts.panel('sill', [(0.0, 0.0), (0.105, 0.0), (0.105, 0.012), (0.0, 0.05)], 0.0, x1 - x0, 0.002)
        xf(sill, Matrix.Translation((x0, gy - 0.015, 0.45)) @ Matrix(((0, 0, 1, 0), (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 0, 1))))
        k.add(sill, 'bronze', BRONZE)
        # the glazing frame: sill stop, head, the mullion, the beads
        k.add(bx('stop_lo', x0, x1, gy - 0.025, gy + 0.012, 0.45, 0.53, 0.003), 'bronze', BRONZE)
        k.add(bx('head', x0, x1, gy - 0.025, gy + 0.03, 2.93, 3.00, 0.003), 'bronze', BRONZE)
        xm = (x0 + x1) / 2
        k.add(bx('mullion', xm - 0.025, xm + 0.025, gy - 0.06, gy + 0.03, 0.53, 2.93, 0.004), 'bronze', BRONZE, dir=2)
        for xa, xb in ((x0, xm - 0.025), (xm + 0.025, x1)):
            gl.append(quad('pane', [(xb, gy, 0.53), (xa, gy, 0.53), (xa, gy, 2.93), (xb, gy, 2.93)]))
            for z in (0.53, 2.93):
                k.add(bx('bead', xa, xb, gy - 0.016, gy - 0.004, z - 0.008, z + 0.008, 0.002), 'bronze', BRONZE)
    # --- the angled returns into the recess: bulkhead, sill, glass, corner posts
    for (p1, p2) in (wret, eret):
        inward = ((p1[0] + p2[0]) / 2 + (0.3 if p1[0] < 30 else -0.3), (p1[1] + p2[1]) / 2 + 0.2)   # a point in the recess
        d = Vector((p2[0] - p1[0], p2[1] - p1[1], 0.0))
        nrm = Vector((-d.y, d.x, 0.0)).normalized()
        if nrm.dot(Vector((inward[0] - p1[0], inward[1] - p1[1], 0.0))) < 0:
            nrm = -nrm
        sgn = 1.0 if Vector((-d.y, d.x, 0.0)).normalized().dot(nrm) > 0 else -1.0
        k.add(seg_box('rbulk', p1, p2, sgn * 0.04, 0.012, 0.05, 0.45, 0.003), 'blackglass', 0x0d0d0e, var=0.5)
        k.add(seg_box('rbulk_back', p1, p2, -sgn * 0.01, 0.06, 0.02, 0.45, 0.0), 'paint', 0x2a2724)
        k.add(seg_box('rshoe', p1, p2, sgn * 0.045, 0.04, 0.0, 0.05, 0.004), 'bronze', BRONZE)
        k.add(seg_box('rsill', p1, p2, sgn * 0.04, 0.10, 0.45, 0.50, 0.004), 'bronze', BRONZE)
        k.add(seg_box('rstop', p1, p2, 0.0, 0.04, 0.50, 0.53, 0.003), 'bronze', BRONZE)
        k.add(seg_box('rhead', p1, p2, 0.0, 0.05, 2.93, 2.97, 0.003), 'bronze', BRONZE)
        gl.append(seg_quad('rpane', p1, p2, 0.53, 2.93, inward))
        for (px, py) in (p1, p2):
            k.add(bx('post', px - 0.035, px + 0.035, py - 0.035, py + 0.035, 0.0, 2.97, 0.006), 'bronze', BRONZE, dir=2)
    # --- the door frame, its transom, the threshold
    for xa, xb in ((DX0 - 0.04, DX0 + 0.02), (DX1 - 0.02, DX1 + 0.04)):
        k.add(bx('djamb', xa, xb, DOOR_PLANE - 0.06, DOOR_PLANE + 0.06, 0.0, 2.95, 0.004), 'bronze', BRONZE, dir=2)
    k.add(bx('dhead', DX0 - 0.04, DX1 + 0.04, DOOR_PLANE - 0.06, DOOR_PLANE + 0.06, DOOR_H, DOOR_H + 0.07, 0.004), 'bronze', BRONZE)
    k.add(bx('dtop', DX0 - 0.04, DX1 + 0.04, DOOR_PLANE - 0.06, DOOR_PLANE + 0.06, 2.90, 2.95, 0.004), 'bronze', BRONZE)
    gl.append(quad('transom', [(DX1 + 0.02, DOOR_PLANE, DOOR_H + 0.07), (DX0 - 0.02, DOOR_PLANE, DOOR_H + 0.07), (DX0 - 0.02, DOOR_PLANE, 2.90),
                               (DX1 + 0.02, DOOR_PLANE, 2.90)]))
    k.add(bx('threshold', DX0, DX1, DOOR_PLANE - 0.07, DOOR_PLANE + 0.07, 0.0, 0.022, 0.006), 'brass', 0xffffff)
    k.add(bx('stopper', 30.5 - 0.02, 30.5 + 0.02, DOOR_PLANE - 0.05, DOOR_PLANE - 0.01, 0.022, 0.04, 0.004), 'brass', 0xffffff)
    # --- the recess: the mosaic floor, the beadboard soffit and its light
    trap = [(wret[0][0] - 0.02, FRONT_Y), (wret[1][0] - 0.03, DOOR_PLANE - 0.02), (eret[1][0] + 0.03, DOOR_PLANE - 0.02),
            (eret[0][0] + 0.02, FRONT_Y)]
    trap_ccw = list(reversed(trap)) if _area2(trap) < 0 else trap
    floor = parts.panel('mosaic', trap_ccw, -0.05, 0.008, 0.0)
    k.add(floor, 'mosaic', 0xffffff)
    soffit = parts.panel('soffit', trap_ccw, 2.95, 3.00, 0.0)
    k.add(soffit, 'paint', 0xd9d2bd)
    for i in range(1, 26):
        xb_ = wret[0][0] + i * (eret[0][0] - wret[0][0]) / 26
        k.add(bx('bead_line', xb_ - 0.002, xb_ + 0.002, DOOR_PLANE, FRONT_Y - 0.01, 2.946, 2.951, 0.0), 'paint', 0xbcb4a0)
    k.add(lathe_axis('can', [(0.0, 2.95), (0.11, 2.95), (0.11, 2.93), (0.095, 2.925), (0.0, 2.925)], M(30.5, 48.95, 0.0), 32), 'paint', 0xe8e4da)
    lamps.append(lathe_axis('can_lens', [(0.0, 2.924), (0.09, 2.924), (0.07, 2.915), (0.0, 2.913)], M(30.5, 48.95, 0.0), 32))
    # --- the transom band: prism-glass tiles in a bronze grid, the bars
    for x0, x1 in ((OX0 + 0.06, OX1 - 0.06),):
        k.add(bx('tbar', x0, x1, gy - 0.03, gy + 0.05, 2.95, 3.00, 0.004), 'bronze', BRONZE)
        # the prism tiles: two rows in a bronze came grid, each its own tile (a few replaced with clear or painted over)
        pt = 0.14
        k.params['prism'] = (x0 + 0.003, 3.003, pt)
        k.add(bx('came_back', x0, x1, gy + 0.004, gy + 0.008, 3.00, 3.28, 0.0), 'bronze', BRONZE)
        rnd_p = random.Random(5)
        ncol = int((x1 - x0) / pt)
        for ci in range(ncol):
            for ri in range(2):
                tx0 = x0 + 0.003 + ci * pt
                tz0 = 3.003 + ri * pt
                kind = rnd_p.random()
                tile = bx('prism', tx0 + 0.003, tx0 + pt - 0.003, gy - 0.006, gy + 0.004, tz0 + 0.003, tz0 + pt - 0.003, 0.0025, 1)
                if kind < 0.04:
                    k.add(tile, 'paint', 0xd8d2bd)           # painted over from inside
                else:
                    k.add(tile, 'prism', 0xb7aec4 if kind > 0.12 else 0xa9b4b8)
        k.add(bx('tbar2', x0, x1, gy - 0.03, gy + 0.05, 3.28, 3.32, 0.004), 'bronze', BRONZE)
        for xi in np.arange(x0 + 0.8, x1 - 0.3, 1.02):
            k.add(bx('tpost', xi - 0.012, xi + 0.012, gy - 0.02, gy + 0.03, 3.00, 3.28, 0.002), 'bronze', BRONZE)
    # --- the sign band: a painted board in a molded frame, three gooseneck lamps over it
    sx0, sx1, sz0, sz1 = OX0, OX1, 3.32, 4.00
    k.add(bx('sign_back', sx0, sx1, FRONT_Y - 0.10, FRONT_Y + 0.005, sz0, sz1, 0.0), 'paint', 0x2a2724)
    k.add(bx('sign_board', sx0 + 0.04, sx1 - 0.04, FRONT_Y, FRONT_Y + 0.02, sz0 + 0.03, sz1 - 0.03, 0.002), 'paint', SIGN_GROUND)
    prof = [(0.0, 0.0), (0.045, 0.0), (0.045, 0.012), (0.03, 0.02), (0.022, 0.04), (0.0, 0.045)]
    for (a, b, c_, d_) in ((sx0, sx1, sz1 - 0.035, 1), (sx0, sx1, sz0 + 0.035, -1)):
        mold = parts.panel('mold', prof, 0.0, b - a, 0.002)
        mold.data.transform(Matrix.Translation((a, FRONT_Y, c_)) @ Matrix(((0, 0, 1, 0), (1, 0, 0, 0), (0, d_, 0, 0), (0, 0, 0, 1))))
        core.orient_normals(mold)
        k.add(mold, 'paint', 0xd9cfb4)
    for xs, sgn in ((sx0, 1), (sx1, -1)):
        k.add(bx('mold_end', xs if sgn > 0 else xs - 0.045, xs + 0.045 if sgn > 0 else xs, FRONT_Y, FRONT_Y + 0.04, sz0, sz1, 0.004), 'paint', 0xd9cfb4)
    k.panel('sign', 12.02, 0.62, draw_sign, density=1.0)
    k.decal('sign', ((sx0 + sx1) / 2, FRONT_Y + 0.0205, (sz0 + sz1) / 2), u=(-1, 0, 0), v=(0, 0, 1), w=sx1 - sx0 - 0.09, h=sz1 - sz0 - 0.07, lift=0.0)
    for gx in (27.0, 30.5, 34.0):
        base = Vector((gx, FRONT_Y + 0.03, sz1 + 0.02))
        k.add(bxc('gn_plate', base, (0.12, 0.02, 0.12), 0.004), 'powder', 0x1d3b36)
        arm = bend([(gx, FRONT_Y + 0.04, sz1 + 0.02), (gx, FRONT_Y + 0.22, sz1 + 0.10), (gx, FRONT_Y + 0.42, sz1 + 0.12), (gx, FRONT_Y + 0.52, sz1 + 0.06)], 0.08, 6)
        k.add(tube('gn_arm', arm, 0.011, 12), 'powder', 0x1d3b36)
        shade = lathe_solid('gn_shade', [(0.012, 0.0), (0.03, -0.01), (0.09, -0.07), (0.13, -0.115), (0.128, -0.12), (0.087, -0.075), (0.026, -0.016),
                                         (0.01, -0.004)], M(gx, FRONT_Y + 0.52, sz1 + 0.07, 0.0, 0.0, -35.0), 36)
        k.add(shade, 'powder', 0x1d3b36)
        bulb = lathe_axis('gn_lens', [(0.0, -0.10), (0.1, -0.10), (0.0, -0.096)], M(gx, FRONT_Y + 0.52, sz1 + 0.07, 0.0, 0.0, -35.0), 24)
        lamps.append(bulb)
    # --- inside the windows: platforms (carpeted), knee walls, the wall returns, window tracks with spots
    wplat = [(IX0, KNEE_Y), (IX0, gy - 0.02), (wx1 - 0.02, gy - 0.02), (DX0 - 0.03, KNEE_Y)]
    eplat = [(DX1 + 0.03, KNEE_Y), (ex0 + 0.02, gy - 0.02), (IX1, gy - 0.02), (IX1, KNEE_Y)]
    for poly in (wplat, eplat):
        pp = list(reversed(poly)) if _area2(poly) < 0 else poly
        k.add(parts.panel('plat', pp, 0.40, PLAT_Z, 0.008), 'carpet', 0x5a2f2a)
    for xa, xb in ((IX0, DX0 - 0.03), (DX1 + 0.03, IX1)):
        k.add(bx('knee', xa, xb, KNEE_Y - 0.10, KNEE_Y, 0.0, 1.05, 0.003), 'paint', 0xd8d0b8)
        k.add(bx('knee_cap', xa, xb, KNEE_Y - 0.13, KNEE_Y + 0.02, 1.05, 1.08, 0.006), 'wood', 0x7a5232, dir=0)
        for xi in np.arange(xa + 0.45, xb - 0.2, 0.9):
            k.add(bx('knee_stile', xi - 0.03, xi + 0.03, KNEE_Y - 0.115, KNEE_Y - 0.1, 0.12, 0.95, 0.004), 'paint', 0xd8d0b8)
        k.add(bx('knee_rail', xa, xb, KNEE_Y - 0.115, KNEE_Y - 0.1, 0.10, 0.16, 0.004), 'paint', 0xd8d0b8)
        k.add(bx('knee_rail', xa, xb, KNEE_Y - 0.115, KNEE_Y - 0.1, 0.90, 0.96, 0.004), 'paint', 0xd8d0b8)
        k.add(bx('knee_base', xa, xb, KNEE_Y - 0.12, KNEE_Y - 0.1, 0.0, 0.10, 0.003), 'lino', 0x2a221c)
    for xa, xb in ((OX0 + 0.06, IX0), (IX1, OX1 - 0.06)):
        k.add(bx('wret', xa, xb, gy - 0.03, gy, 0.0, CZ, 0.0), 'wallpaint', 0xd8d2bc)
    for x0, x1 in ((IX0 + 0.3, DX0 - 0.5), (DX1 + 0.5, IX1 - 0.3)):
        ty = 48.95
        k.add(bx('track', x0, x1, ty - 0.017, ty + 0.017, CZ - 0.035, CZ - 0.005, 0.003), 'powder', 0x161616)
        for sx in np.linspace(x0 + 0.5, x1 - 0.5, 3):
            k.add(bx('spot_stem', sx - 0.012, sx + 0.012, ty - 0.012, ty + 0.012, CZ - 0.09, CZ - 0.035, 0.002), 'powder', 0x161616)
            aim = M(sx, ty, CZ - 0.11, 0.0, 0.0, 25.0)
            k.add(xf(lathe_solid('spot_can', [(0.0, 0.06), (0.04, 0.06), (0.042, 0.0), (0.038, -0.08), (0.034, -0.085), (0.0, -0.085)], segments=24), aim),
                  'powder', 0x161616)
            lamps.append(xf(lathe_axis('spot_lens', [(0.0, -0.0855), (0.03, -0.0855), (0.0, -0.083)], segments=24), aim))
    # --- window dressing: a hard suitcase, stacked books, a bentwood chair, a vase
    wz = PLAT_Z
    k.add(bx('suitcase', 25.6, 26.28, 48.95, 49.14, wz, wz + 0.46, 0.02), 'vinyl', 0x6b4a32)
    for zz in (wz + 0.06, wz + 0.40):
        k.add(bx('sc_band', 25.59, 26.29, 48.94, 49.15, zz - 0.012, zz + 0.012, 0.004), 'leather', 0x3a2618)
    k.add(tube('sc_handle', bend([(25.86, 49.045, wz + 0.46), (25.88, 49.045, wz + 0.50), (26.02, 49.045, wz + 0.50), (26.04, 49.045, wz + 0.46)], 0.015), 0.008, 8),
          'leather', 0x2a1a10)
    for i, (hh, col) in enumerate(((0.045, 0x6a2a2a), (0.035, 0x23384a), (0.05, 0x5a5236), (0.03, 0x2a4a34))):
        zb = wz + sum(x for x, _ in ((0.045, 0), (0.035, 0), (0.05, 0), (0.03, 0))[:i])
        k.add(bx('book', 27.1 + i * 0.01, 27.36 + i * 0.012, 48.80, 49.0 - i * 0.01, zb, zb + hh, 0.004), 'cardboard', col)
    k.add(lathe_axis('vase', [(0.0, 0.0), (0.05, 0.0), (0.075, 0.08), (0.05, 0.2), (0.032, 0.26), (0.04, 0.28), (0.0, 0.28)], M(27.25, 48.9, wz + 0.16), 32),
          'gloss', 0x3a6a8a)
    chair_x, chair_y = 33.3, 48.95
    k.add(xf(lathe_axis('ch_seat', [(0.0, 0.43), (0.21, 0.43), (0.215, 0.45), (0.2, 0.46), (0.0, 0.46)], segments=32), M(chair_x, chair_y, wz)), 'wood', 0x4a2c18)
    for a in (45, 135, 225, 315):
        ca, sa = math.cos(math.radians(a)), math.sin(math.radians(a))
        k.add(tube('ch_leg', [(chair_x + ca * 0.15, chair_y + sa * 0.15, wz + 0.43), (chair_x + ca * 0.2, chair_y + sa * 0.2, wz + 0.01)], 0.013, 10), 'wood', 0x4a2c18)
    k.add(tube('ch_back', bend([(chair_x - 0.17, chair_y + 0.08, wz + 0.45), (chair_x - 0.2, chair_y + 0.05, wz + 0.85), (chair_x - 0.14, chair_y - 0.08, wz + 0.95),
                                (chair_x + 0.0, chair_y - 0.12, wz + 0.96), (chair_x + 0.14, chair_y - 0.08, wz + 0.95), (chair_x + 0.2, chair_y + 0.05, wz + 0.85),
                                (chair_x + 0.17, chair_y + 0.08, wz + 0.45)], 0.08), 0.012, 10), 'wood', 0x4a2c18)
    k.add(tube('ch_ring', [(chair_x + 0.17 * math.cos(a), chair_y + 0.17 * math.sin(a), wz + 0.18) for a in np.linspace(0, 2 * math.pi, 40, endpoint=False)], 0.008, 8,
               closed=True), 'wood', 0x4a2c18)
    # --- the posters taped inside the glass (they read from the street), the house number on the transom
    k.panel('yellow', 0.43, 0.56, draw_poster_yellow)
    k.panel('donate', 0.43, 0.56, draw_poster_donate)
    k.panel('paperback', 0.20, 0.26, draw_paper_back)
    k.panel('number', 0.30, 0.12, draw_number)
    k.decal('yellow', (27.95, gy - 0.004, 1.42), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0, back='paperback')
    k.decal('donate', (33.05, gy - 0.004, 1.42), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0, back='paperback')
    k.decal('number', (30.5, DOOR_PLANE - 0.004, 2.60), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0)
    k.grime += [((30.5, 48.4, 1.0), 0.6, 0.5)]
    obj = k.finish(angle=40.0)
    gobj = core.join(gl, 'SM_NS_Thrift_Front_Glass')
    gobj.name = gobj.data.name = 'SM_NS_Thrift_Front_Glass'
    core.assign(gobj, UNB['ns_glass'])
    gobj['ns_unbaked'] = True
    print(f'[{MODULE}] SM_NS_Thrift_Front_Glass: {sum(len(p.vertices) - 2 for p in gobj.data.polygons)} triangles, slots [\'ns_glass\']')
    glow_o = glow(lamps, 'SM_NS_Thrift_Front_GlowWarm')
    return [obj, gobj, glow_o]


def _area2(poly):
    return sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(poly, poly[1:] + poly[:1]))


# ------------------------------------------------------------------ the room (SITE)

WALL_UP = 0xd9d3bd      # cream above the chair rail
WALL_LO = 0x7f8a70      # sage below it


def draw_exit(s, w, h):
    s.rect(0, 0, w, h, 0xf2efe6, rough=0.3)
    s.rect(3, 3, w - 6, h - 6, 0xfbfaf6, rough=0.2)
    s.mid('EXIT', w / 2, h * 0.52, h * 0.62, w * 0.8, 0xd01c1c, 'Black', tracking=1.15)


def draw_staff(s, w, h):
    s.rect(0, 0, w, h, 0x1d2a3a, rough=0.4)
    s.mid('STAFF ONLY', w / 2, h * 0.66, h * 0.28, w - 16, 0xf2efe6, 'Black', tracking=1.15)
    s.mid('DONATIONS RECEIVED IN BACK', w / 2, h * 0.27, h * 0.11, w - 16, 0xd8d0b8, 'Bold', tracking=1.1)


def draw_stencil(s, w, h):
    s.rect(0, 0, w, h, 0x6f7a80, rough=0.5)
    rnd = random.Random(3)
    s.mid('DONATIONS', w / 2, h * 0.5, h * 0.62, w - 10, 0xe8e2d0, 'Black', tracking=1.25)
    for i in range(14):
        s.rect(rnd.uniform(0, w), rnd.uniform(0, h), rnd.uniform(4, 30), rnd.uniform(2, 8), 0x6f7a80, rough=0.5, alpha=0.8)


def draw_nofood(s, w, h):
    s.rect(0, 0, w, h, 0xf6f3ea, rough=0.8)
    hand_mid(s, 'ALL SALES FINAL', w / 2, h * 0.7, 26, w - 20, 0xb0161c, width=0.11, seed=12)
    hand_mid(s, 'no food or drinks please', w / 2, h * 0.42, 15, w - 24, 0x1a1a1a, width=0.09, seed=13)
    hand_mid(s, 'thank you - Bea', w / 2, h * 0.16, 12, w - 40, 0x1b2f7a, width=0.08, seed=14)


def draw_tape_s(s, w, h):
    s.rect(0, 0, w, h, 0xd9cfae, rough=0.6, alpha=0.8)


def ext_sign(k, center, fy, hang=None, group='trim'):
    """An EXIT sign: a white box with red letters lit from inside (the print slot's glow), its face toward fy * Y (both
    faces when it hangs from the ceiling by a stem to `hang`)."""
    cx, cy, cz = center
    k.add(bx('exit_box', cx - 0.17, cx + 0.17, cy - 0.025, cy + 0.025, cz - 0.11, cz + 0.11, 0.008), 'plastic', 0xefece4, var=0.6, group=group)
    for sgn in ((fy, -fy) if hang else (fy,)):
        k.decal('exit', (cx, cy + sgn * 0.0256, cz), u=(-sgn, 0, 0), v=(0, 0, 1), w=0.30, h=0.18, lift=0.0)
    if hang:
        k.add(rod('exit_stem', (cx, cy, cz + 0.11), (cx, cy, hang), 0.008, 10), 'paint', 0xe8e6df, group=group)
        k.add(lathe_axis('exit_can', [(0.0, hang - 0.02), (0.06, hang - 0.02), (0.06, hang), (0.0, hang)], M(cx, cy, 0.0), 20), 'paint', 0xe8e6df,
              group=group)


def shelving(k, x0, x1, y0, y1, z1, shelves, seed, group='trim'):
    """Grey steel boltless shelving against a wall (posts, beams, particle-board decks) with boxes and bags on it."""
    rnd = random.Random(seed)
    for (px, py) in ((x0, y0), (x1, y0), (x0, y1), (x1, y1)):
        k.add(bx('post', px - 0.02, px + 0.02, py - 0.02, py + 0.02, 0.0, z1, 0.002), 'powder', 0x5f6366, dir=2, group=group)
    for z in shelves:
        k.add(bx('deck', x0, x1, y0, y1, z - 0.016, z, 0.002), 'particle', 0xffffff, group=group)
        for (a, b) in ((y0, y0 + 0.03), (y1 - 0.03, y1)):
            k.add(bx('beam', x0, x1, a, b, z - 0.05, z - 0.016, 0.002), 'powder', 0x5f6366, group=group)
        # what's on it: cartons, bags of donations, folded piles
        yy = y0 + 0.04
        while yy < y1 - 0.25:
            w = rnd.uniform(0.28, 0.55)
            if yy + w > y1 - 0.03:
                break
            kind = rnd.random()
            h_ = rnd.uniform(0.18, 0.34)
            if kind < 0.5:
                k.add(bx('box', x0 + 0.02, x1 - rnd.uniform(0.02, 0.08), yy, yy + w, z, z + h_, 0.006), 'cardboard', rnd.choice((0xa47f52, 0x9b7a50, 0xb08a5c)),
                      group=group)
            elif kind < 0.8:
                # a bag of donated clothes: a slumped sack, its neck twisted and knotted on top
                bh = h_ * 1.25
                b = soft_slab('bag', x1 - x0 - 0.06, w * 0.95, bh, 0.06, 6, 6, 6, sag=0.0, seed=rnd.randint(1, 99), noise_amt=0.05,
                              matrix=M((x0 + x1) / 2, yy + w / 2, z))
                for v in b.data.vertices:          # narrowing to the neck, slumped out at the foot
                    t = (v.co.z - z) / bh
                    sx = 1.0 + 0.12 * (1 - t) - 0.55 * max(0.0, t - 0.7) / 0.3
                    v.co.x = (x0 + x1) / 2 + (v.co.x - (x0 + x1) / 2) * sx
                    v.co.y = yy + w / 2 + (v.co.y - yy - w / 2) * sx
                bc = rnd.choice((0x1c1c1e, 0xe8e6de, 0x2e4a7a))
                k.add(b, 'plastic', bc, group=group, var=0.0)
                k.add(blob('knot', ((x0 + x1) / 2, yy + w / 2, z + bh + 0.02), (0.05, 0.06, 0.05), seed=rnd.randint(1, 99), detail=0.25, subdiv=2), 'plastic', bc,
                      group=group, var=0.0)
            else:
                for j in range(rnd.randint(2, 5)):
                    k.add(soft_slab('pile', x1 - x0 - 0.1, w * 0.9, 0.03, 0.01, 6, 6, 2, sag=0.004, seed=rnd.randint(1, 99), noise_amt=0.004,
                                    matrix=M((x0 + x1) / 2, yy + w / 2, z + j * 0.03, rnd.uniform(-5, 5))), 'cloth', rnd.choice(TINTS), group=group)
            yy += w + rnd.uniform(0.02, 0.08)


def shell():
    """SM_NS_Thrift_Shell + _GlowCool."""
    k = Kit('ths', 'SM_NS_Thrift_Shell', TEXTURE_SIZES, dust=0.3, low_grime=0.12, groups=['floor', 'wall', 'ceil', 'trim'])
    k.cavity = 0.02
    k.params['vct'] = {
        'tile': 0.3048, 'origin': (IX0 + 0.13, IY0 + 0.07), 'border': 0.3048, 'border_col': 0x3d3a33, 'room': (IX0, IX1, IY0, KNEE_Y),
        'patch': 0x9e9988, 'worn': 0xb4ad9c,
        'paths': [([(30.5, 48.3), (30.5, 44.5), (30.4, 37.2)], 1.3), ([(30.4, 46.0), (28.3, 44.2), (27.9, 42.6)], 1.0),
                  ([(30.5, 43.0), (33.1, 41.5), (34.5, 37.8)], 1.0), ([(30.5, 45.2), (32.0, 46.4), (34.4, 46.6)], 0.9),
                  ([(30.5, 41.0), (27.6, 39.0), (26.0, 37.4)], 0.9)],
    }
    k.params['vct_walls'] = (IX0, IX1, IY0, KNEE_Y)
    lamps = []
    # --- the floor: VCT (one face, so its bake is all floor)
    k.add(quad('floor', [(IX0, IY0, 0.0), (IX1, IY0, 0.0), (IX1, KNEE_Y + 0.02, 0.0), (IX0, KNEE_Y + 0.02, 0.0)]), 'vct', 0x8d8a7b, group='floor', keep_uv=True)
    # --- the walls: sage below a chair rail, cream above, a vinyl cove base
    def wall_quads(p0, p1, z0, z1, col, holes=()):
        (ax, ay), (bxx, byy) = p0, p1
        segs = [(0.0, 1.0)]
        L = math.hypot(bxx - ax, byy - ay)
        for (h0, h1, hz) in holes:
            new = []
            for s0, s1 in segs:
                t0, t1 = h0 / L, h1 / L
                if t1 <= s0 or t0 >= s1:
                    new.append((s0, s1))
                else:
                    if t0 > s0:
                        new.append((s0, t0))
                    if t1 < s1:
                        new.append((t1, s1))
                    if z1 > hz:
                        k.add(quad('wall', [(ax + (bxx - ax) * t1, ay + (byy - ay) * t1, max(z0, hz)), (ax + (bxx - ax) * t0, ay + (byy - ay) * t0, max(z0, hz)),
                                            (ax + (bxx - ax) * t0, ay + (byy - ay) * t0, z1), (ax + (bxx - ax) * t1, ay + (byy - ay) * t1, z1)]),
                              'wallpaint', col, group='wall')
            segs = new
        for s0, s1 in segs:
            k.add(quad('wall', [(ax + (bxx - ax) * s1, ay + (byy - ay) * s1, z0), (ax + (bxx - ax) * s0, ay + (byy - ay) * s0, z0),
                                (ax + (bxx - ax) * s0, ay + (byy - ay) * s0, z1), (ax + (bxx - ax) * s1, ay + (byy - ay) * s1, z1)]),
                  'wallpaint', col, group='wall')

    walls = [((IX0, GLASS_Y - 0.03), (IX0, IY0), ()),           # west, seen from inside (+x): run north -> south
             ((IX0, IY0), (IX1, IY0), ((BACK_DOOR[0] - IX0, BACK_DOOR[1] - IX0, BACK_DOOR[2]),)),   # back: west -> east
             ((IX1, IY0), (IX1, GLASS_Y - 0.03), ())]           # east: south -> north
    for p0, p1, holes in walls:
        wall_quads(p0, p1, 0.10, 0.95, WALL_LO, holes)
        wall_quads(p0, p1, 0.98, CZ, WALL_UP, holes)
        d = Vector((p1[0] - p0[0], p1[1] - p0[1], 0.0)).normalized()
        n = Vector((-d.y, d.x, 0.0))
        L = math.hypot(p1[0] - p0[0], p1[1] - p0[1])
        cuts = [(0.0, L)]
        for (h0, h1, hz) in holes:
            cuts = [(0.0, h0 - 0.06), (h1 + 0.06, L)]
        for a, b in cuts:
            q0 = Vector((p0[0], p0[1], 0.0)) + d * a
            q1 = Vector((p0[0], p0[1], 0.0)) + d * b
            k.add(seg_box('base', (q0.x, q0.y), (q1.x, q1.y), 0.006, 0.012, 0.0, 0.10, 0.003), 'lino', 0x2b2520, group='wall')
            k.add(seg_box('rail', (q0.x, q0.y), (q1.x, q1.y), 0.012, 0.024, 0.93, 0.995, 0.005), 'wood', 0x6a4a2e, dir=1 if abs(d.y) > 0.5 else 0,
                  group='wall')
            k.add(seg_box('wall_infill', (q0.x, q0.y), (q1.x, q1.y), 0.0, 0.002, 0.95, 0.98, 0.0), 'wallpaint', WALL_LO, group='wall')
            k.add(seg_box('wall_infill', (q0.x, q0.y), (q1.x, q1.y), 0.0, 0.002, 0.0, 0.10, 0.0), 'wallpaint', WALL_LO, group='wall')
    # --- the donation door at the back: a hollow-metal frame, a painted leaf with a push bar, a closer, the signs
    x0, x1, dh = BACK_DOOR
    for xa, xb in ((x0 - 0.05, x0), (x1, x1 + 0.05)):
        k.add(bx('frame', xa, xb, IY0 - 0.02, IY0 + 0.025, 0.0, dh + 0.05, 0.004), 'paint', 0x5d666b, dir=2, group='trim')
    k.add(bx('frame_head', x0 - 0.05, x1 + 0.05, IY0 - 0.02, IY0 + 0.025, dh, dh + 0.05, 0.004), 'paint', 0x5d666b, group='trim')
    k.add(bx('leaf', x0 + 0.003, x1 - 0.003, IY0 - 0.03, IY0 + 0.012, 0.01, dh - 0.003, 0.004), 'paint', 0x6f7a80, group='trim')
    k.add(bx('kick', x0 + 0.02, x1 - 0.02, IY0 + 0.012, IY0 + 0.014, 0.02, 0.27, 0.002), 'steel', 0xffffff, group='trim')
    for xb_ in (x0 + 0.08, x1 - 0.08):
        k.add(bx('bar_mount', xb_ - 0.04, xb_ + 0.04, IY0 + 0.012, IY0 + 0.06, 0.96, 1.04, 0.01), 'powder', 0x2a2a2a, group='trim')
    k.add(bx('pushbar', x0 + 0.1, x1 - 0.1, IY0 + 0.04, IY0 + 0.07, 0.975, 1.025, 0.012), 'steel', 0xffffff, dir=0, group='trim')
    k.add(bx('closer', x0 + 0.1, x0 + 0.42, IY0 + 0.012, IY0 + 0.07, dh - 0.12, dh - 0.05, 0.01), 'powder', 0x8a8c8e, group='trim')
    k.add(rod('closer_arm', (x0 + 0.38, IY0 + 0.05, dh - 0.08), (x0 + 0.62, IY0 + 0.05, dh + 0.02), 0.008, 8), 'powder', 0x8a8c8e, group='trim')
    k.panel('stencil', 0.6, 0.14, draw_stencil)
    k.panel('staff', 0.30, 0.16, draw_staff)
    k.panel('exit', 0.30, 0.18, draw_exit, glow=2.5)
    k.panel('nofood', 0.28, 0.20, draw_nofood)
    k.panel('tape', 0.05, 0.018, draw_tape_s)
    k.decal('stencil', ((x0 + x1) / 2, IY0 + 0.0125, 1.42), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0005)     # faces +Y, into the room
    k.decal('staff', ((x0 + x1) / 2, IY0 + 0.0125, 1.68), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0005)
    ext_sign(k, ((x0 + x1) / 2, IY0 + 0.05, dh + 0.24), 1)
    k.add(bx('exit_mount', (x0 + x1) / 2 - 0.12, (x0 + x1) / 2 + 0.12, IY0, IY0 + 0.025, dh + 0.18, dh + 0.30, 0.004), 'plastic', 0xefece4, group='trim')
    ext_sign(k, (30.5, KNEE_Y - 0.45, 2.75), -1, hang=CZ)
    # the notice by the counter, taped up on the west wall (its room side faces +X)
    k.decal('nofood', (IX0 + 0.003, 44.0, 1.55), u=(0, 1, 0), v=(0, 0, 1), lift=0.0)
    for dz in (0.09, -0.09):
        k.decal('tape', (IX0 + 0.0035, 44.0 + 0.12, 1.55 + dz), u=(0, 1, 0), v=(0, 0, 1), w=0.045, h=0.016, lift=0.0)
        k.decal('tape', (IX0 + 0.0035, 44.0 - 0.12, 1.55 + dz), u=(0, 1, 0), v=(0, 0, 1), w=0.045, h=0.016, lift=0.0)
    # fire extinguisher, pull station, light switches, thermostat, outlets
    ex_x = x0 - 0.45
    k.add(lathe_axis('ext', [(0.0, 0.62), (0.075, 0.62), (0.078, 0.64), (0.078, 1.02), (0.06, 1.07), (0.02, 1.1), (0.0, 1.1)], M(ex_x, IY0 + 0.1, 0.0), 32),
          'paint', 0xb3161a, group='trim')
    k.add(bx('ext_head', ex_x - 0.02, ex_x + 0.02, IY0 + 0.08, IY0 + 0.12, 1.1, 1.16, 0.008), 'steel', 0xffffff, group='trim')
    k.add(tube('ext_hose', bend([(ex_x + 0.01, IY0 + 0.12, 1.14), (ex_x + 0.09, IY0 + 0.15, 1.12), (ex_x + 0.09, IY0 + 0.17, 0.80), (ex_x + 0.08, IY0 + 0.17, 0.72)], 0.03),
               0.009, 10), 'rubber', 0x111111, group='trim')
    k.add(bx('ext_bracket', ex_x - 0.05, ex_x + 0.05, IY0, IY0 + 0.03, 0.95, 1.0, 0.004), 'powder', 0x2a2a2a, group='trim')
    k.add(bx('pull', x1 + 0.25, x1 + 0.38, IY0, IY0 + 0.04, 1.15, 1.32, 0.01), 'plastic', 0xc11a1a, var=0.2, group='trim')
    for sx_, sz_ in ((x1 + 0.3, 1.2 - 0.12), (x0 - 0.9, 1.2)):
        k.add(bx('switch', sx_ - 0.04, sx_ + 0.04, IY0, IY0 + 0.008, sz_ - 0.06, sz_ + 0.06, 0.003), 'plastic', 0xeeeae0, var=0.8, group='trim')
    k.add(bx('thermo', x0 - 1.4, x0 - 1.28, IY0, IY0 + 0.03, 1.45, 1.55, 0.01), 'plastic', 0xe6e2d8, var=0.9, group='trim')
    for oy in (39.5, 43.0, 46.5):
        k.add(bx('outlet', IX1 - 0.008, IX1, oy - 0.04, oy + 0.04, 0.28, 0.40, 0.003), 'plastic', 0xeeeae0, var=0.8, group='trim')
    # --- the pegboard on the east wall: a framed panel, hooks with belts, bags and scarves on them
    py0, py1, pz0, pz1 = 44.30, 47.30, 1.00, 2.20
    k.add(bx('peg', IX1 - 0.006, IX1, py0, py1, pz0, pz1, 0.0), 'pegboard', 0x8a6a48, dir=1, group='trim')
    for (a, b, c_, d_) in ((py0 - 0.03, py1 + 0.03, pz0 - 0.03, pz0), (py0 - 0.03, py1 + 0.03, pz1, pz1 + 0.03), (py0 - 0.03, py0, pz0, pz1), (py1, py1 + 0.03, pz0, pz1)):
        k.add(bx('peg_frame', IX1 - 0.022, IX1, a, b, c_, d_, 0.004), 'wood', 0x7a5232, dir=1 if (b - a) > (d_ - c_) else 2, group='trim')
    rnd = random.Random(44)
    for row, hz in enumerate((2.02, 1.62, 1.26)):
        for col_i in range(7):
            hy = py0 + 0.22 + col_i * 0.42 + rnd.uniform(-0.03, 0.03)
            k.add(tube('hook', bend([(IX1 - 0.004, hy, hz), (IX1 - 0.02, hy, hz), (IX1 - 0.14, hy, hz + 0.03), (IX1 - 0.15, hy, hz + 0.06)], 0.01), 0.003, 6),
                  'chrome', var=0.5, group='trim')
            kind = rnd.random()
            if kind < 0.4:      # belts looped over the hook
                for j in range(rnd.randint(1, 3)):
                    ph = rnd.uniform(-0.3, 0.3)
                    pts = [(IX1 - 0.03 - 0.10 * (1 - abs(t)) - j * 0.012, hy + 0.05 * math.sin(t * 2.8 + ph), hz - 0.28 * (1 - t * t)) for t in np.linspace(-1, 1, 14)]
                    belt = core.sweep('belt', pts, [(0.0, -0.016), (0.003, -0.016), (0.003, 0.016), (0.0, 0.016)])
                    k.add(belt, 'leather', rnd.choice((0x3a2414, 0x15120f, 0x5a3a20)), group='trim')
            elif kind < 0.75:   # a purse by its strap
                k.add(soft_slab('purse', 0.07, 0.24, 0.17, 0.03, 4, 8, 6, sag=0.0, seed=rnd.randint(1, 99), noise_amt=0.004,
                                matrix=M(IX1 - 0.09, hy, hz - 0.36, 90)), 'vinyl', rnd.choice((0x5a2a20, 0x1a1a1a, 0x8a6a40, 0x2a3a5a)), group='trim')
                k.add(tube('strap', [(IX1 - 0.12, hy - 0.08, hz - 0.19), (IX1 - 0.14, hy - 0.03, hz + 0.04), (IX1 - 0.14, hy + 0.03, hz + 0.04), (IX1 - 0.12, hy + 0.08, hz - 0.19)],
                           0.004, 6), 'leather', 0x2a1a10, group='trim')
            elif kind < 0.92:   # a scarf hanging double
                k.add(soft_slab('scarf', 0.012, 0.11, 0.48, 0.004, 2, 4, 10, seed=rnd.randint(1, 99), noise_amt=0.01, matrix=M(IX1 - 0.13, hy, hz - 0.46)),
                      'cloth', rnd.choice(TINTS), group='trim')
    # --- the staff shelving behind the counter, donations waiting to be sorted
    shelving(k, IX0 + 0.02, IX0 + 0.47, 42.25, 45.0, 2.13, (0.12, 0.62, 1.12, 1.62, 2.10), 61)
    # --- the drop ceiling: lay-in tiles (one face each, so each can be stained or newer), the grid, troffers, a diffuser,
    #     sprinkler heads, a smoke detector; one tile pushed up out of the grid
    skip = set()
    for tx, ty in TROFFERS:
        skip.add((round(tx, 3), round(ty, 3)))
    rnd = random.Random(7)
    ajar = (9, 4)
    for i in range(len(GRID_X) - 1):
        for j in range(len(GRID_Y) - 1):
            ax, bxx = GRID_X[i], GRID_X[i + 1]
            ay, byy = GRID_Y[j], GRID_Y[j + 1]
            cx_, cy_ = (ax + bxx) / 2, (ay + byy) / 2
            if any(abs(cx_ - tx) < 0.2 and abs(cy_ - ty) < 0.5 for tx, ty in TROFFERS):
                continue
            z = CZ + 0.006
            if (i, j) == ajar:
                t = quad('tile', [(ax, ay, z + 0.05), (ax, byy, z + 0.02), (bxx, byy, z + 0.02), (bxx, ay, z + 0.05)])
                k.add(bx('plenum', ax - 0.1, bxx + 0.1, ay - 0.1, byy + 0.1, CZ + 0.4, CZ + 0.42, 0.0), 'concrete', 0x1a1816, group='ceil')
            else:
                t = quad('tile', [(ax, ay, z), (ax, byy, z), (bxx, byy, z), (bxx, ay, z)])
            k.add(t, 'ceiltile', 0xe6e2d6, group='ceil', var=rnd.random())
    for gx in GRID_X[1:-1]:
        k.add(bx('main_tee', gx - 0.012, gx + 0.012, IY0, GLASS_Y - 0.02, CZ, CZ + 0.006, 0.001), 'paint', 0xe9e6dd, dir=1, group='ceil')
        k.add(bx('main_web', gx - 0.003, gx + 0.003, IY0, GLASS_Y - 0.02, CZ + 0.006, CZ + 0.04, 0.0), 'paint', 0xe9e6dd, dir=1, group='ceil')
    for gy_ in GRID_Y[1:-1]:
        k.add(bx('cross_tee', IX0, IX1, gy_ - 0.012, gy_ + 0.012, CZ - 0.0005, CZ + 0.0055, 0.001), 'paint', 0xe9e6dd, group='ceil')
    for (a, b, c_, d_) in ((IX0, IX0 + 0.024, IY0, GLASS_Y - 0.02), (IX1 - 0.024, IX1, IY0, GLASS_Y - 0.02), (IX0, IX1, IY0, IY0 + 0.024)):
        k.add(bx('angle', a, b, c_, d_, CZ - 0.001, CZ + 0.006, 0.001), 'paint', 0xe9e6dd, group='ceil')
    for tx, ty in TROFFERS:
        hw, hd = 0.305, 0.61
        for (a, b, c_, d_) in ((tx - hw, tx + hw, ty - hd, ty - hd + 0.03), (tx - hw, tx + hw, ty + hd - 0.03, ty + hd),
                               (tx - hw, tx - hw + 0.03, ty - hd, ty + hd), (tx + hw - 0.03, tx + hw, ty - hd, ty + hd)):
            k.add(bx('trof_flange', a, b, c_, d_, CZ - 0.002, CZ + 0.006, 0.002), 'paint', 0xeceae3, group='ceil')
        for sgn in (-1, 1):
            k.add(seg_quad('trof_side', (tx + sgn * (hw - 0.03), ty - hd + 0.03), (tx + sgn * (hw - 0.03), ty + hd - 0.03), CZ + 0.006, CZ + 0.05,
                           (tx, ty)), 'paint', 0xf0eee8, group='ceil')
            k.add(seg_quad('trof_end', (tx - hw + 0.03, ty + sgn * (hd - 0.03)), (tx + hw - 0.03, ty + sgn * (hd - 0.03)), CZ + 0.006, CZ + 0.05,
                           (tx, ty)), 'paint', 0xf0eee8, group='ceil')
        lens = quad('trof_lens', [(tx - hw + 0.03, ty - hd + 0.03, CZ + 0.05), (tx - hw + 0.03, ty + hd - 0.03, CZ + 0.05),
                                  (tx + hw - 0.03, ty + hd - 0.03, CZ + 0.05), (tx + hw - 0.03, ty - hd + 0.03, CZ + 0.05)])
        lamps.append(lens)
    # a square supply diffuser and an eggcrate return in two cells, sprinkler heads, the smoke detector
    for (i, j, kind) in ((3, 3, 'supply'), (15, 7, 'supply'), (9, 9, 'return')):
        ax, bxx, ay, byy = GRID_X[i], GRID_X[i + 1], GRID_Y[j], GRID_Y[j + 1]
        cx_, cy_ = (ax + bxx) / 2, (ay + byy) / 2
        if kind == 'supply':
            for r_, dz in ((0.29, 0.0), (0.22, 0.012), (0.15, 0.024), (0.08, 0.036)):
                k.add(bx('diff', cx_ - r_, cx_ + r_, cy_ - r_, cy_ + r_, CZ - 0.01 + dz, CZ - 0.002 + dz, 0.004), 'paint', 0xefede6, group='ceil')
        else:
            k.add(bx('ret', cx_ - 0.29, cx_ + 0.29, cy_ - 0.29, cy_ + 0.29, CZ + 0.004, CZ + 0.008, 0.0), 'concrete', 0x2a2826, group='ceil')
            for t_ in np.linspace(-0.27, 0.27, 12):
                k.add(bx('egg', cx_ + t_ - 0.002, cx_ + t_ + 0.002, cy_ - 0.28, cy_ + 0.28, CZ - 0.002, CZ + 0.008, 0.0), 'paint', 0xe8e5dc, group='ceil')
                k.add(bx('egg', cx_ - 0.28, cx_ + 0.28, cy_ + t_ - 0.002, cy_ + t_ + 0.002, CZ - 0.002, CZ + 0.008, 0.0), 'paint', 0xe8e5dc, group='ceil')
    for sx_ in (27.4, 30.5, 33.6):
        for sy_ in (38.5, 42.1, 45.7):
            k.add(lathe_axis('sprinkler', [(0.0, CZ - 0.05), (0.012, CZ - 0.048), (0.006, CZ - 0.035), (0.009, CZ - 0.01), (0.028, CZ - 0.004), (0.03, CZ),
                                           (0.0, CZ)], M(sx_, sy_, 0.0), 16), 'brass', 0xffffff, group='ceil')
    k.add(lathe_axis('smoke', [(0.0, CZ - 0.045), (0.06, CZ - 0.045), (0.07, CZ - 0.03), (0.07, CZ), (0.0, CZ)], M(29.0, 41.5, 0.0), 32), 'plastic', 0xefece6,
          group='ceil', var=0.7)
    k.grime += [((x0 + 0.1, IY0, 1.0), 0.35, 0.6), ((IX1, 46.0, 1.4), 0.6, 0.25)]
    obj = k.finish(angle=40.0)
    return [obj, glow(lamps, 'SM_NS_Thrift_Shell_GlowCool')]


# ------------------------------------------------------------------ the door leaves (PROP)

LEAF_T = 0.045


def draw_push(s, w, h):
    s.rect(0, 0, w, h, 0xb8964a, metal=1.0, rough=0.28)
    s.rect(3, 3, w - 6, h - 6, 0xc9a85a, metal=1.0, rough=0.22)
    s.mid('PUSH', w / 2, h / 2, h * 0.32, w * 0.7, 0x2a2010, 'Black', tracking=1.3, metal=0.6, rough=0.5)
    for x in (8, w - 8):
        for y in (10, h - 10):
            s.circle(x, y, 2.4, 0x8a7036, metal=1.0, rough=0.35, n=16)


def draw_agnes(s, w, h):
    """St. Agnes Outreach's round window decal (white vinyl, cut lettering, a lamb-and-heart mark)."""
    cx, cy, r = w / 2, h / 2, w / 2
    s.circle(cx, cy, r, 0xf4f1e8, rough=0.4, n=96)
    s.circle(cx, cy, r - 5, 0x1f4a44, rough=0.4, n=96)
    s.circle(cx, cy, r - 30, 0xf4f1e8, rough=0.4, n=96)

    s.mid('ST. AGNES', cx, cy + r * 0.62, 15, r * 1.1, 0xf4f1e8, 'Black', tracking=1.3)
    s.mid('OUTREACH', cx, cy - r * 0.64, 15, r * 1.1, 0xf4f1e8, 'Black', tracking=1.3)
    # a heart with a cross in it
    hr = r * 0.28
    s.circle(cx - hr * 0.42, cy + hr * 0.2, hr * 0.52, 0xb0432e, n=48)
    s.circle(cx + hr * 0.42, cy + hr * 0.2, hr * 0.52, 0xb0432e, n=48)
    s.poly([(cx - hr * 0.92, cy + hr * 0.05), (cx + hr * 0.92, cy + hr * 0.05), (cx, cy - hr * 1.0)], 0xb0432e)
    s.rect(cx - 2.5, cy - hr * 0.55, 5, hr * 1.0, 0xf4f1e8)
    s.rect(cx - hr * 0.3, cy + hr * 0.12, hr * 0.6, 5, 0xf4f1e8)
    s.mid('SINCE 1987', cx, cy - r * 0.42, 9, r * 0.9, 0x1f4a44, 'Bold', tracking=1.2)


def door_leaf(hand=1):
    """SM_NS_Thrift_Door (hand 1: spans +Y from its hinge) and _R (hand -1: spans -Y): a narrow-stile bronze storefront
    leaf with plate glass, a brass push plate outside, a pull inside, a brass kick plate, three butt hinges."""
    name = 'SM_NS_Thrift_Door' if hand > 0 else 'SM_NS_Thrift_Door_R'
    k = Kit('thd' if hand > 0 else 'thdr', name, TEXTURE_SIZES, dust=0.15, low_grime=0.25)
    W, Hh, T = LEAF_W, DOOR_H - 0.01, LEAF_T
    y0, y1 = (0.006, W - 0.003) if hand > 0 else (-(W - 0.003), -0.006)
    sy = 1.0 if hand > 0 else -1.0
    hinge_side = y0 if hand > 0 else y1
    lock_side = y1 if hand > 0 else y0
    stile, top, bot = 0.095, 0.11, 0.26
    z0 = 0.008
    xs = (-T / 2, T / 2)
    # the frame of the leaf: stiles, rails, and the glass stops
    k.add(bx('stile_h', xs[0], xs[1], min(hinge_side, hinge_side + sy * stile), max(hinge_side, hinge_side + sy * stile), z0, Hh, 0.004), 'bronze', BRONZE, dir=2)
    k.add(bx('stile_l', xs[0], xs[1], min(lock_side, lock_side - sy * stile), max(lock_side, lock_side - sy * stile), z0, Hh, 0.004), 'bronze', BRONZE, dir=2)
    k.add(bx('rail_t', xs[0], xs[1], y0, y1, Hh - top, Hh, 0.004), 'bronze', BRONZE, dir=1)
    k.add(bx('rail_b', xs[0], xs[1], y0, y1, z0, z0 + bot, 0.004), 'bronze', BRONZE, dir=1)
    gy0, gy1 = (y0 + stile, y1 - stile) if hand > 0 else (y0 + stile, y1 - stile)
    for xx in (xs[0] - 0.006, xs[1] + 0.006):
        for (a, b, c_, d_) in ((gy0, gy1, z0 + bot, z0 + bot + 0.018), (gy0, gy1, Hh - top - 0.018, Hh - top), (gy0, gy0 + 0.018, z0 + bot, Hh - top),
                               (gy1 - 0.018, gy1, z0 + bot, Hh - top)):
            k.add(bx('gstop', xx - 0.006, xx + 0.006, a, b, c_, d_, 0.002), 'bronze', BRONZE)
    glass = quad('glass', [(0.0, gy0, z0 + bot), (0.0, gy1, z0 + bot), (0.0, gy1, Hh - top), (0.0, gy0, Hh - top)])
    k.special(glass, UNB['ns_glass'])
    # kick plates both sides, push plate outside (-X), pull inside
    for xx, sgn in ((xs[0] - 0.0015, -1), (xs[1] + 0.0015, 1)):
        k.add(bx('kick', xx - 0.0015, xx + 0.0015, y0 + 0.01, y1 - 0.01, z0 + 0.01, z0 + 0.24, 0.001), 'brass', 0xffffff, dir=1)
    py = lock_side - sy * 0.05
    k.panel('push', 0.09, 0.30, draw_push)
    k.add(bx('push_plate', xs[0] - 0.003, xs[0], py - 0.045, py + 0.045, 0.92, 1.22, 0.002), 'brass', 0xffffff, dir=2)
    k.decal('push', (xs[0] - 0.0031, py, 1.07), u=(0, -1, 0), v=(0, 0, 1), lift=0.0)
    for zz in (0.88, 1.18):
        k.add(rod('pull_post', (xs[1], py, zz), (xs[1] + 0.06, py, zz), 0.009, 12), 'brass', 0xffffff)
    k.add(rod('pull', (xs[1] + 0.06, py, 0.84), (xs[1] + 0.06, py, 1.22), 0.0125, 16), 'brass', 0xffffff, dir=2)
    for zz in (0.25, 1.05, 1.85):
        k.add(bx('hinge', xs[0] - 0.004, xs[1] + 0.004, hinge_side - sy * 0.006, hinge_side + sy * 0.004, zz, zz + 0.11, 0.002) if hand > 0 else
              bx('hinge', xs[0] - 0.004, xs[1] + 0.004, hinge_side - 0.004, hinge_side + 0.006, zz, zz + 0.11, 0.002), 'brass', 0xffffff, dir=2)
        k.add(rod('knuckle', (0.0, 0.0, zz), (0.0, 0.0, zz + 0.11), 0.008, 12), 'brass', 0xffffff, dir=2)
    k.add(bx('lockcyl', xs[0] - 0.008, xs[0], lock_side - sy * 0.05 - 0.016, lock_side - sy * 0.05 + 0.016, 1.38, 1.42, 0.006), 'brass', 0xffffff)
    if hand > 0:
        k.panel('agnes', 0.20, 0.20, draw_agnes)
        disc = [(0.5 + 0.5 * math.cos(2 * math.pi * i / 40), 0.5 + 0.5 * math.sin(2 * math.pi * i / 40)) for i in range(40)]
        k.decal('agnes', (-0.0025, (gy0 + gy1) / 2, 1.58), u=(0, -1, 0), v=(0, 0, 1), lift=0.0, outline=disc)
    k.grime += [((xs[0], py, 1.07), 0.18, 0.6), ((xs[1], py, 1.0), 0.2, 0.5)]
    k.polish += [((xs[0], py, 1.07), 0.08, 0.8)]
    return [k.finish(angle=40.0, unbaked_order=['ns_glass'])]


# ------------------------------------------------------------------ the register counter (PROP)

CT_L, CT_D, CT_H = 1.83, 0.61, 0.97
OAK = 0x8a5e34


def draw_buttons(s, w, h):
    """A jar's worth of buttons, packed: every color, two and four holes, a few shanks."""
    rnd = random.Random(31)
    s.rect(0, 0, w, h, 0x3a3028)
    cols = [0xe8e2d0, 0x1a1a1a, 0x8a1e22, 0x2a4a7a, 0xc9a24a, 0x3a6a3a, 0x7a4a2a, 0xd8c8a8, 0x6a3a6a, 0xb8b8b8, 0xe07a3a, 0x2e2e3a]
    for i in range(520):
        x, y, r = rnd.uniform(0, w), rnd.uniform(0, h), rnd.uniform(4.0, 9.5)
        c = rnd.choice(cols)
        s.circle(x, y, r, _tone(c, 0.7), rough=0.35, n=20)
        s.circle(x, y, r * 0.86, c, rough=0.3, n=20)
        if rnd.random() < 0.8:
            for hx, hy in ((-0.22, -0.22), (0.22, 0.22), (-0.22, 0.22), (0.22, -0.22))[:rnd.choice((2, 4))]:
                s.circle(x + hx * r, y + hy * r, r * 0.11, _tone(c, 0.4), rough=0.5, n=8)


def draw_paytent(s, w, h):
    s.rect(0, 0, w, h, 0xf6f2e6, rough=0.85)
    hand_mid(s, 'PAY HERE', w / 2, h * 0.62, 26, w - 20, 0x1f4a44, width=0.12, seed=21)
    hand_mid(s, 'God bless - St. Agnes', w / 2, h * 0.28, 13, w - 24, 0xb0161c, width=0.08, seed=22)


def draw_keys(s, w, h):
    s.rect(0, 0, w, h, 0x34322e, rough=0.5)
    rows = [['CLR', 'VOID', 'RTN', 'SUB'], ['7', '8', '9', 'DEPT'], ['4', '5', '6', 'TAX'], ['1', '2', '3', 'X'], ['0', '00', '.', 'CASH']]
    kw, kh = (w - 10) / 4, (h - 10) / 5
    for r_, row in enumerate(rows):
        for c_, lab in enumerate(row):
            x0, y0 = 5 + c_ * kw, h - 5 - (r_ + 1) * kh
            col = 0xc4161c if lab == 'CASH' else (0xd9d2c0 if lab[0].isdigit() or lab in ('00', '.') else 0x7a8a96)
            s.rect(x0 + 1.5, y0 + 1.5, kw - 3, kh - 3, col, rough=0.4)
            s.mid(lab, x0 + kw / 2, y0 + kh / 2, kh * 0.34, kw - 6, 0x141414 if col == 0xd9d2c0 else 0xffffff, 'Black')


def draw_vfd(s, w, h):
    s.rect(0, 0, w, h, 0x05120f, rough=0.08)
    s.mid('THANK YOU', w / 2, h * 0.7, h * 0.3, w * 0.9, 0x63f0c8, 'Bold', rough=0.08)
    s.mid('ST. AGNES', w / 2, h * 0.28, h * 0.3, w * 0.9, 0x63f0c8, 'Bold', rough=0.08)


def draw_bagstamp(s, w, h):
    s.rect(0, 0, w, h, 0xa8825a, rough=0.85)
    s.circle(w / 2, h * 0.55, w * 0.32, 0x1f4a44, rough=0.85, alpha=0.75)
    s.circle(w / 2, h * 0.55, w * 0.27, 0xa8825a, rough=0.85)
    s.mid('SECOND', w / 2, h * 0.64, w * 0.09, w * 0.5, 0x1f4a44, 'Black', alpha=0.8)
    s.mid('CHANCE', w / 2, h * 0.53, w * 0.09, w * 0.5, 0x1f4a44, 'Black', alpha=0.8)
    s.mid('THRIFT', w / 2, h * 0.42, w * 0.08, w * 0.5, 0x1f4a44, 'Black', alpha=0.8)


def draw_pantry(s, w, h):
    s.rect(0, 0, w, h, 0xf2ede0, rough=0.6)
    s.rect(0, h * 0.72, w, h * 0.28, 0x1f4a44, rough=0.5)
    s.mid('ST. AGNES', w / 2, h * 0.86, h * 0.12, w - 6, 0xf2ede0, 'Black')
    s.mid('FOOD', w / 2, h * 0.56, h * 0.16, w - 6, 0xb0432e, 'Black')
    s.mid('PANTRY', w / 2, h * 0.38, h * 0.16, w - 6, 0xb0432e, 'Black')
    s.mid('every coin helps', w / 2, h * 0.16, h * 0.08, w - 6, 0x2a2a2a, 'Bold')


def draw_reg_back(s, w, h):
    """The register's back panel: molded vent slots, a foil model plate (a made-up maker), a serial sticker."""
    s.rect(0, 0, w, h, 0xc3bcab, rough=0.55)
    for i in range(14):
        x0 = w * 0.06 + i * w * 0.033
        s.rect(x0, h * 0.42, w * 0.016, h * 0.42, 0x1a1916, rough=0.7)
        s.rect(x0, h * 0.42, w * 0.004, h * 0.42, 0x5a564e, rough=0.6, alpha=0.6)
    s.rect(w * 0.6, h * 0.48, w * 0.32, h * 0.36, 0xa9adb0, metal=0.9, rough=0.35)
    s.mid('REGISTRON', w * 0.76, h * 0.72, h * 0.1, w * 0.28, 0x1a1a1a, 'Black')
    s.mid('ER-310  120V 60Hz', w * 0.76, h * 0.57, h * 0.06, w * 0.28, 0x2a2a2a, 'Bold')
    s.rect(w * 0.08, h * 0.1, w * 0.22, h * 0.2, 0xf2efe6, rough=0.8)
    s.mid('SN 0447-2291', w * 0.19, h * 0.2, h * 0.07, w * 0.2, 0x2a2a2a, 'Bold')


def cash_register(k, x, y, z, yaw=0.0):
    """An old electronic register (keys toward +X, the staff; the customer's pole display toward -X)."""
    m = M(x, y, z, yaw)
    k.add(xf(bx('reg_drawer', -0.21, 0.21, -0.2, 0.2, 0.0, 0.106, 0.014), m), 'plastic', 0xbdb6a6, var=0.8)
    k.add(xf(bx('reg_drawer_front', 0.205, 0.215, -0.17, 0.17, 0.015, 0.095, 0.004), m), 'plastic', 0xa9a294, var=0.8)
    k.add(xf(bx('reg_pull', 0.21, 0.222, -0.08, 0.08, 0.072, 0.086, 0.005), m), 'plastic', 0x8f897c, var=0.8)
    k.add(xf(rod('reg_lock', (0.215, 0.12, 0.055), (0.225, 0.12, 0.055), 0.008, 12), m), 'chrome')
    k.add(xf(bx('reg_seam', -0.195, 0.2, -0.185, 0.185, 0.104, 0.114, 0.003), m), 'plastic', 0x3a3833, var=0.2)
    # the smoked printer cover behind the operator's display, its tear bar, and the feet
    k.add(xf(bx('reg_printer', -0.175, -0.035, 0.125, 0.19, 0.245, 0.272, 0.006), m), 'gloss', 0x1d1f21)
    k.add(xf(bx('reg_tear', -0.11, -0.085, 0.13, 0.185, 0.27, 0.275, 0.001), m), 'steel')
    for fx in (-0.18, 0.18):
        for fy in (-0.17, 0.17):
            k.add(xf(bx('reg_foot', fx - 0.02, fx + 0.02, fy - 0.02, fy + 0.02, -0.004, 0.002, 0.002), m), 'rubber', 0x141414)
    # the back, which is what the customer sees: vent slots and the model plate
    k.panel('reg_back', 0.30, 0.09, draw_reg_back)
    k.decal('reg_back', tuple(m @ Vector((-0.1906, 0.0, 0.18))), u=tuple(m.to_3x3() @ Vector((0, -1, 0))), v=tuple(m.to_3x3() @ Vector((0, 0, 1))), w=0.30, h=0.09,
            lift=0.0)
    body = parts.panel('reg_body', [(-0.19, 0.11), (0.08, 0.11), (0.19, 0.11), (0.19, 0.15), (0.0, 0.25), (-0.19, 0.25)], -0.18, 0.18, 0.006)
    body.data.transform(Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1))))
    k.add(xf(body, m), 'plastic', 0xc8c1b0, var=0.85)
    kp = 'reg_keys'
    k.panel(kp, 0.20, 0.17, draw_keys)
    # the keyboard on the slope (from (0.19, 0.15) up to (0.0, 0.25)), reading for someone at +X
    slope_c = Vector((0.095, 0.0, 0.2005))
    u = Vector((0.0, 1.0, 0.0))
    v = Vector((-0.19, 0.0, 0.10)).normalized()

    n = u.cross(v).normalized()
    k.decal(kp, tuple(m @ (slope_c + n * 0.0015)), u=tuple(m.to_3x3() @ u), v=tuple(m.to_3x3() @ v), w=0.20, h=0.17, lift=0.0)
    k.add(xf(bx('reg_disp', -0.17, -0.06, -0.12, 0.12, 0.25, 0.30, 0.004), m), 'plastic', 0xc8c1b0, var=0.85)
    k.panel('vfd', 0.16, 0.035, draw_vfd, glow=1.5)
    k.decal('vfd', tuple(m @ Vector((-0.0595, 0.0, 0.275))), u=tuple(m.to_3x3() @ Vector((0, 1, 0))), v=tuple(m.to_3x3() @ Vector((0, 0, 1))), w=0.15, h=0.03, lift=0.0)
    k.add(xf(rod('pole', (-0.14, 0.0, 0.30), (-0.14, 0.0, 0.42), 0.012, 12), m), 'plastic', 0x2a2a2a)
    k.add(xf(bx('pole_disp', -0.17, -0.11, -0.08, 0.08, 0.42, 0.47, 0.006), m), 'plastic', 0x2a2a2a)
    k.decal('vfd', tuple(m @ Vector((-0.1705, 0.0, 0.445))), u=tuple(m.to_3x3() @ Vector((0, -1, 0))), v=tuple(m.to_3x3() @ Vector((0, 0, 1))), w=0.14, h=0.03,
            lift=0.0)
    # the receipt curling out of the printer at the back-left
    pts = [(-0.1 + 0.02 * t, 0.15, 0.25 + 0.06 * math.sin(t * 2.0)) for t in np.linspace(0, 1.4, 10)]
    paper = core.sweep('receipt', [tuple(m @ Vector(p)) for p in pts], [(0.0, -0.028), (0.0005, -0.028), (0.0005, 0.028), (0.0, 0.028)])
    k.add(paper, 'paper', 0xf2efe8)


def xf_dir(m, v):
    return m.to_3x3() @ Vector(v)


def tag_gun(k, x, y, z, yaw):
    """A pistol-grip tag gun lying on its side: the body, a red trigger, the needle."""
    m = M(x, y, z, yaw)
    body = parts.panel('tg_body', [(0.0, 0.0), (0.03, 0.0), (0.05, 0.09), (0.13, 0.11), (0.15, 0.13), (0.13, 0.15), (0.02, 0.15), (-0.01, 0.12)], 0.0, 0.028, 0.005)
    k.add(xf(body, m), 'plastic', 0x2a2a2e, var=0.2)
    k.add(xf(rod('tg_needle', (0.15, 0.13, 0.014), (0.2, 0.13, 0.014), 0.0015, 6), m), 'steel')
    k.add(xf(bx('tg_trigger', 0.04, 0.06, 0.06, 0.1, 0.004, 0.024, 0.003), m), 'plastic', 0xc8321e)


def counter():
    """SM_NS_Thrift_Counter: the old five-and-dime's oak counter (customer side -X), worn speckled laminate top; on it the
    register, a tag gun, paper bags, a jar of buttons, a hand-lettered PAY HERE card, a call bell, the pantry can; behind,
    shelves of bags and the church-station radio."""
    k = Kit('thn', 'SM_NS_Thrift_Counter', TEXTURE_SIZES, dust=0.25, low_grime=0.18)
    hl, hd = CT_L / 2, CT_D / 2
    # carcass: a recessed toe kick, the paneled oak front and ends, the top
    k.add(bx('kick', -hd + 0.07, hd, -hl + 0.02, hl - 0.02, 0.0, 0.10, 0.002), 'lino', 0x1e1a16)
    k.add(bx('front', -hd, -hd + 0.025, -hl, hl, 0.10, CT_H - 0.04, 0.003), 'wood', OAK, dir=1)
    for yy in np.linspace(-hl + 0.32, hl - 0.32, 3):
        k.add(bx('panel', -hd - 0.012, -hd, yy - 0.26, yy + 0.26, 0.24, CT_H - 0.16, 0.006), 'wood', OAK, dir=2)
        k.add(bx('panel_in', -hd - 0.02, -hd - 0.012, yy - 0.22, yy + 0.22, 0.28, CT_H - 0.20, 0.01), 'wood', _tone(OAK, 0.92), dir=2)
    k.add(bx('base_mold', -hd - 0.02, -hd + 0.03, -hl - 0.02, hl + 0.02, 0.10, 0.16, 0.008), 'wood', _tone(OAK, 0.85), dir=1)
    for ys in (-1, 1):
        k.add(bx('end', -hd, hd, ys * hl - (0.025 if ys > 0 else 0.0), ys * hl + (0.0 if ys > 0 else 0.025), 0.10, CT_H - 0.04, 0.003), 'wood', OAK, dir=0)
    k.add(bx('top', -hd - 0.03, hd + 0.01, -hl - 0.03, hl + 0.03, CT_H - 0.04, CT_H, 0.006), 'laminate', 0xc9c0a8, var=0.9)
    k.add(bx('top_edge', -hd - 0.035, -hd - 0.025, -hl - 0.03, hl + 0.03, CT_H - 0.045, CT_H + 0.002, 0.004), 'wood', _tone(OAK, 0.8), dir=1)
    # the staff side: two open shelves with bags, tag rolls, a phone book; the radio on top
    for zs in (0.12, 0.48):
        k.add(bx('shelf', -hd + 0.03, hd - 0.02, -hl + 0.03, hl - 0.03, zs, zs + 0.02, 0.003), 'plywood', 0x9a7a52, dir=1)
    k.add(bx('back_div', -hd + 0.025, -hd + 0.035, -hl, hl, 0.10, CT_H - 0.04, 0.0), 'plywood', 0x8a6a46)
    rnd = random.Random(12)
    for i in range(14):
        k.add(bx('bags', 0.0 + 0.002 * i, 0.27 + 0.002 * i, -0.8, -0.38, 0.14 + i * 0.006, 0.145 + i * 0.006, 0.002), 'paper', 0xa8825a)
    for j, yy in enumerate((0.1, 0.2, 0.3)):
        k.add(xf(lathe_axis('tagroll', [(0.0, 0.0), (0.045, 0.0), (0.045, 0.05), (0.0, 0.05)], segments=24), M(0.12, yy, 0.50)), 'paper', (0xf2efe6, 0xf2d24a, 0xe8a0a0)[j])
    k.add(bx('phonebook', -0.05, 0.2, 0.45, 0.72, 0.50, 0.56, 0.006), 'paper', 0xd8c640)
    k.add(bx('shoebox', -0.1, 0.22, 0.4, 0.78, 0.14, 0.26, 0.004), 'cardboard', 0xc8a070)
    # radio on the counter's back corner
    rx, ry = 0.16, 0.68
    k.add(bx('radio', rx - 0.06, rx + 0.06, ry - 0.14, ry + 0.14, CT_H, CT_H + 0.15, 0.015), 'plastic', 0x6a4a32, var=0.5)
    k.add(bx('radio_grille', rx - 0.062, rx - 0.058, ry - 0.12, ry + 0.02, CT_H + 0.02, CT_H + 0.13, 0.004), 'chrome', var=0.4)
    k.add(xf(lathe_axis('radio_dial', [(0.0, 0.0), (0.03, 0.0), (0.03, 0.012), (0.0, 0.014)], segments=24), M(rx - 0.06, ry + 0.08, CT_H + 0.075, 0, -90)),
          'plastic', 0xe8dcc0)
    k.add(rod('radio_ant', (rx, ry + 0.12, CT_H + 0.15), (rx - 0.18, ry + 0.2, CT_H + 0.55), 0.002, 6), 'chrome')
    k.add(tube('radio_handle', bend([(rx, ry - 0.1, CT_H + 0.15), (rx, ry - 0.1, CT_H + 0.19), (rx, ry + 0.1, CT_H + 0.19), (rx, ry + 0.1, CT_H + 0.15)], 0.02), 0.006, 8),
          'chrome')
    # the register, the tag gun and a fan of tags, the bags, the jar, the cards, the bell, the can
    cash_register(k, 0.04, 0.0, CT_H, 0.0)
    tag_gun(k, -0.05, -0.42, CT_H + 0.0, 20)
    for i in range(8):
        k.add(xf(bx('tag', -0.01, 0.01, -0.025, 0.025, 0.0, 0.0006, 0.0), M(-0.12 + i * 0.006, -0.30 + i * 0.012, CT_H + 0.001 + i * 0.0006, rnd.uniform(-30, 30))),
              'paper', 0xf2efe6)
    k.panel('bagstamp', 0.18, 0.2, draw_bagstamp)
    open_bag = parts.panel('openbag', core.rounded_rect(-0.09, -0.06, 0.09, 0.06, 0.004, 2), CT_H, CT_H + 0.30, 0.0)
    k.add(xf(open_bag, M(-0.15, 0.62, 0.0, 15)), 'paper', 0xa8825a)
    k.decal('bagstamp', tuple(M(-0.15, 0.62, 0.0, 15) @ Vector((-0.0905, 0.0, CT_H + 0.16))), u=tuple(M(0, 0, 0, 15).to_3x3() @ Vector((0, -1, 0))), v=(0, 0, 1),
            w=0.11, h=0.12, lift=0.0)
    jx, jy = -0.17, 0.32
    k.panel('buttons', 0.30, 0.10, draw_buttons)
    jar = core.lathe('jar_fill', [(0.048, 0.004), (0.048, 0.10)], 40)
    jar.data.transform(M(jx, jy, CT_H))
    k.uv_print(jar, 'buttons')
    k.add(lathe_axis('jar_bottom', [(0.0, 0.0), (0.052, 0.0), (0.052, 0.006), (0.0, 0.006)], M(jx, jy, CT_H), 40), 'glassbaked', 0x9ab0a8)
    k.add(lathe_solid('jar_glass_top', [(0.049, 0.10), (0.053, 0.10), (0.053, 0.125), (0.049, 0.125)], M(jx, jy, CT_H), 40), 'glassbaked', 0x9ab0a8)
    k.add(lathe_axis('jar_lid', [(0.0, 0.125), (0.054, 0.125), (0.056, 0.13), (0.056, 0.142), (0.05, 0.146), (0.0, 0.146)], M(jx, jy, CT_H), 40), 'paint', 0xc9a24a)
    k.add(lathe_axis('jar_buttons_top', [(0.0, 0.10), (0.047, 0.10), (0.03, 0.106), (0.0, 0.108)], M(jx, jy, CT_H), 40), 'plastic', 0x8a6a5a, var=0.0)
    k.panel('paytent', 0.15, 0.09, draw_paytent)
    tent_c = Vector((-0.22, -0.12, CT_H))
    sa, ca = math.sin(math.radians(20)), math.cos(math.radians(20))
    for sgn in (1, -1):
        k.add(xf(bx('tent', -0.0004, 0.0004, -0.075, 0.075, 0.0, 0.095, 0.0), Matrix.Translation(tent_c + Vector((-sgn * 0.0325, 0.0, 0.0))) @
                 Matrix.Rotation(sgn * math.radians(20), 4, 'Y')), 'paper', 0xf6f2e6)
    face_c = tent_c + Vector((-0.0325 + 0.0475 * sa, 0.0, 0.0475 * ca))     # the -X panel's middle
    k.decal('paytent', tuple(face_c + Vector((-ca, 0.0, sa)) * 0.0009), u=(0, -1, 0), v=(sa, 0, ca), w=0.146, h=0.088, lift=0.0)
    k.add(lathe_axis('bell_base', [(0.0, 0.0), (0.04, 0.0), (0.04, 0.012), (0.0, 0.012)], M(-0.2, 0.15, CT_H), 32), 'plastic', 0x1a1a1a)
    k.add(lathe_axis('bell', [(0.0, 0.012), (0.042, 0.014), (0.038, 0.04), (0.02, 0.058), (0.0, 0.062)], M(-0.2, 0.15, CT_H), 32), 'chrome', var=0.2)
    k.add(lathe_axis('bell_btn', [(0.0, 0.062), (0.006, 0.062), (0.006, 0.075), (0.0, 0.077)], M(-0.2, 0.15, CT_H), 12), 'chrome')
    k.panel('pantry', 0.22, 0.12, draw_pantry)
    can = core.lathe('can', [(0.05, 0.002), (0.05, 0.12)], 40)
    can.data.transform(M(-0.2, 0.5, CT_H, 200))
    k.uv_print(can, 'pantry')
    k.add(lathe_axis('can_lid', [(0.0, 0.12), (0.052, 0.12), (0.052, 0.126), (0.0, 0.126)], M(-0.2, 0.5, CT_H), 40), 'galv', 0xffffff)
    k.add(lathe_axis('can_base', [(0.0, 0.0), (0.051, 0.0), (0.051, 0.004), (0.0, 0.004)], M(-0.2, 0.5, CT_H), 40), 'galv', 0xffffff)
    # a pen cup and a spike of receipts
    k.add(lathe_axis('cup', [(0.0, 0.0), (0.035, 0.0), (0.038, 0.1), (0.0, 0.1)], M(0.22, -0.25, CT_H), 24), 'gloss', 0x2a4a7a)
    for i, (dx, dy, col) in enumerate(((0.006, 0.004, 0x1a1a1a), (-0.008, 0.01, 0x1b2f7a), (0.004, -0.012, 0xb0161c))):
        k.add(rod('pen', (0.22 + dx, -0.25 + dy, CT_H + 0.03), (0.22 + dx * 3, -0.25 + dy * 3, CT_H + 0.155), 0.0045, 8), 'plastic', col)
    k.add(rod('spike', (0.22, 0.32, CT_H), (0.22, 0.32, CT_H + 0.14), 0.0015, 6), 'steel')
    k.add(lathe_axis('spike_base', [(0.0, 0.0), (0.035, 0.0), (0.03, 0.012), (0.0, 0.014)], M(0.22, 0.32, CT_H), 24), 'paint', 0x1a1a1a)
    for i in range(6):
        k.add(xf(bx('slip', -0.035, 0.035, -0.06, 0.06, 0.0, 0.0005, 0.0), M(0.22, 0.32, CT_H + 0.03 + i * 0.008, rnd.uniform(-25, 25), rnd.uniform(-6, 6))),
              'paper', 0xf2efe6)
    k.grime += [((-hd, 0.0, CT_H), 0.5, 0.35), ((-hd - 0.02, 0.6, 0.6), 0.4, 0.3)]
    k.polish += [((-hd + 0.1, 0.0, CT_H), 0.45, 0.7)]
    return [k.finish(angle=40.0)]


# ------------------------------------------------------------------ the glass case (PROP)

CS_L, CS_D, CS_H, CS_DECK = 1.22, 0.51, 0.97, 0.62


def sunglasses(k, c, yaw, col=0x111111, lens=0x0c0f12, folded=True, scale=1.0):
    """A pair of sunglasses lying on their temples (the lenses toward -X of their own frame), at c, turned yaw."""
    m = M(c[0], c[1], c[2], yaw) @ Matrix.Diagonal((scale, scale, scale, 1.0))
    for s in (-1, 1):
        out = core.rounded_rect(-0.026, -0.021, 0.026, 0.021, 0.014, 4)
        rim = parts.outline_prism('sg_rim', out, [core.rounded_rect(-0.022, -0.017, 0.022, 0.017, 0.011, 4)], 0.0, 0.004)
        rim.data.transform(M(0.0, s * 0.033, 0.024, 0.0) @ Matrix.Rotation(math.radians(90), 4, 'Y') @ Matrix.Rotation(math.radians(90), 4, 'Z'))
        k.add(xf(rim, m), 'gloss', col)
        ln = parts.panel('sg_lens', core.rounded_rect(-0.0225, -0.0175, 0.0225, 0.0175, 0.011, 4), 0.0, 0.0015)
        ln.data.transform(M(0.001, s * 0.033, 0.024, 0.0) @ Matrix.Rotation(math.radians(90), 4, 'Y') @ Matrix.Rotation(math.radians(90), 4, 'Z'))
        k.add(xf(ln, m), 'glassbaked', lens)
        if folded:
            k.add(xf(tube('sg_temple', [(0.004, s * 0.058, 0.032), (0.012, s * 0.06, 0.032), (0.018, s * 0.04, 0.018), (0.016, -s * 0.03, 0.012)], 0.0022, 6), m),
                  'gloss', col)
        else:
            k.add(xf(tube('sg_temple', [(0.004, s * 0.058, 0.032), (0.03, s * 0.062, 0.03), (0.12, s * 0.064, 0.026), (0.135, s * 0.062, 0.012)], 0.0022, 6), m),
                  'gloss', col)
    k.add(xf(tube('sg_bridge', [(0.0, -0.008, 0.036), (-0.002, 0.0, 0.039), (0.0, 0.008, 0.036)], 0.002, 6), m), 'gloss', col)


def bead_chain(k, pts, r, col, fin='gloss', step=None):
    """A necklace of beads along a path (each bead a small sphere)."""
    path = core.catmull_rom(pts, 6)
    acc = 0.0
    step = step or r * 2.1
    last = Vector(path[0])
    beads = []
    for p in path[1:]:
        p = Vector(p)
        acc += (p - last).length
        last = p
        if acc >= step:
            acc = 0.0
            bm = bmesh.new()
            bmesh.ops.create_uvsphere(bm, u_segments=8, v_segments=6, radius=r)
            o = core.mesh_object('bead', bm)
            o.data.transform(Matrix.Translation(p))
            beads.append(o)
    if beads:
        b = core.join(beads, 'beads')
        b.data.shade_smooth()
        k.add(b, fin, col)


def watch(k, c, yaw, metal='steel', strap=0x2a1a10, face=0xe8e2d0):
    m = M(c[0], c[1], c[2], yaw)
    k.add(xf(lathe_axis('w_case', [(0.0, 0.0), (0.019, 0.0), (0.02, 0.006), (0.018, 0.011), (0.0, 0.011)], segments=32), m @ Matrix.Rotation(math.radians(70), 4, 'Y')),
          metal, 0xffffff)
    k.add(xf(lathe_axis('w_face', [(0.0, 0.0111), (0.016, 0.0111), (0.0, 0.0112)], segments=32), m @ Matrix.Rotation(math.radians(70), 4, 'Y')), 'gloss', face)
    loop = [(0.0, 0.03 * math.cos(a), 0.03 * math.sin(a) - 0.0) for a in np.linspace(0, 2 * math.pi, 28, endpoint=False)]
    k.add(xf(core.sweep('w_strap', loop, [(-0.009, -0.0015), (0.009, -0.0015), (0.009, 0.0015), (-0.009, 0.0015)], closed=True), m @ M(-0.012, 0, 0.0)), 'leather', strap)


def draw_askbea(s, w, h):
    s.rect(0, 0, w, h, 0xf7f3e8, rough=0.85)
    hand_mid(s, 'ASK TO SEE', w / 2, h * 0.64, 22, w - 16, 0x1a1a1a, width=0.11, seed=41)
    hand_mid(s, 'Bea has the key', w / 2, h * 0.27, 14, w - 20, 0x1b2f7a, width=0.08, seed=42)


def case():
    """SM_NS_Thrift_Case: a jewelry showcase on an oak base (customer side -X): glass top, front and ends, sliding glass
    doors on the staff side; inside, on navy velvet: sunglasses, costume necklaces on a bust, brooches, bangles, watches."""
    k = Kit('thc', 'SM_NS_Thrift_Case', TEXTURE_SIZES, dust=0.2, low_grime=0.15)
    hl, hd = CS_L / 2, CS_D / 2
    k.add(bx('kick', -hd + 0.06, hd - 0.02, -hl + 0.02, hl - 0.02, 0.0, 0.10, 0.002), 'lino', 0x1e1a16)
    k.add(bx('base', -hd, hd, -hl, hl, 0.10, CS_DECK - 0.02, 0.004), 'wood', OAK, dir=1)
    for yy in (-hl / 2, hl / 2):
        k.add(bx('panel', -hd - 0.012, -hd, yy - 0.25, yy + 0.25, 0.18, CS_DECK - 0.1, 0.006), 'wood', OAK, dir=2)
    k.add(bx('base_top', -hd - 0.01, hd + 0.01, -hl - 0.01, hl + 0.01, CS_DECK - 0.02, CS_DECK, 0.004), 'wood', _tone(OAK, 0.85), dir=1)
    # the deck in velvet, the aluminum frame of the glass box
    k.add(bx('deck', -hd + 0.02, hd - 0.02, -hl + 0.02, hl - 0.02, CS_DECK, CS_DECK + 0.012, 0.006), 'velvet', 0x1d2a4a)
    f = 0.018
    for (xa, ya) in ((-hd, -hl), (-hd, hl), (hd, -hl), (hd, hl)):
        k.add(bx('post', xa - (0 if xa < 0 else f), xa + (f if xa < 0 else 0), ya - (0 if ya < 0 else f), ya + (f if ya < 0 else 0), CS_DECK, CS_H, 0.002),
              'alu', 0xffffff, dir=2)
    for z in (CS_DECK, CS_H - f):
        for xa in (-hd, hd - f):
            k.add(bx('rail', xa, xa + f, -hl, hl, z, z + f, 0.002), 'alu', 0xffffff, dir=1)
        for ya in (-hl, hl - f):
            k.add(bx('rail', -hd, hd, ya, ya + f, z, z + f, 0.002), 'alu', 0xffffff, dir=0)
    gz0, gz1 = CS_DECK + f, CS_H - f
    glass = [quad('g_front', [(-hd + 0.006, hl - f, gz0), (-hd + 0.006, -hl + f, gz0), (-hd + 0.006, -hl + f, gz1), (-hd + 0.006, hl - f, gz1)]),
             quad('g_top', [(-hd + f, -hl + f, CS_H - 0.006), (hd - f, -hl + f, CS_H - 0.006), (hd - f, hl - f, CS_H - 0.006), (-hd + f, hl - f, CS_H - 0.006)])]
    for sgn in (-1, 1):
        y_ = sgn * (hl - 0.006)
        glass.append(quad('g_end', [(-hd + f, y_, gz0), (hd - f, y_, gz0), (hd - f, y_, gz1), (-hd + f, y_, gz1)]))
    for j, (ya, yb, xx) in enumerate(((-hl + f, 0.02, hd - 0.012), (-0.02, hl - f, hd - 0.022))):
        glass.append(quad('g_door', [(xx, ya, gz0), (xx, yb, gz0), (xx, yb, gz1), (xx, ya, gz1)]))
        k.add(bx('door_pull', xx - 0.004, xx + 0.004, (ya + yb) / 2 - 0.04, (ya + yb) / 2 + 0.04, gz0 + 0.06, gz0 + 0.08, 0.002), 'chrome', var=0.3)
    for g in glass:
        k.special(g, UNB['ns_glass'])
    k.add(bx('lock', hd - 0.006, hd + 0.004, -0.01, 0.01, gz0 + 0.02, gz0 + 0.06, 0.003), 'chrome', var=0.4)
    z0 = CS_DECK + 0.012
    rnd = random.Random(55)
    # sunglasses in a row near the front, two on a little riser
    for i, yy in enumerate(np.linspace(-0.48, -0.08, 4)):
        sunglasses(k, (-0.16 + rnd.uniform(-0.01, 0.01), yy, z0), rnd.uniform(-8, 8), col=rnd.choice((0x111111, 0x5a3a20, 0x8a6a3a, 0x1a2a4a)))
    k.add(bx('riser', -0.02, 0.12, -0.42, -0.12, z0, z0 + 0.06, 0.004), 'velvet', 0x1d2a4a)
    for yy in (-0.36, -0.18):
        sunglasses(k, (-0.01, yy, z0 + 0.06), 0, col=0x2a2a2a, folded=False)
    # a velvet bust with two necklaces
    bxp, byp = 0.08, 0.2
    # a jewelry neck form: a flat bust (shoulders, a neck) in black velvet on a round foot
    k.add(lathe_axis('bust_foot', [(0.0, 0.0), (0.06, 0.0), (0.06, 0.01), (0.015, 0.02), (0.012, 0.06), (0.0, 0.06)], M(bxp, byp, z0), 32), 'brass', 0xffffff)
    form = loft_z('bust', [(z0 + 0.30, 0.022, 0.018), (z0 + 0.27, 0.026, 0.02), (z0 + 0.235, 0.03, 0.022), (z0 + 0.215, 0.085, 0.028), (z0 + 0.195, 0.115, 0.03),
                           (z0 + 0.16, 0.12, 0.03), (z0 + 0.11, 0.105, 0.028), (z0 + 0.07, 0.07, 0.022), (z0 + 0.055, 0.06, 0.02)], n=36, p=0.7, yc=byp, xc=bxp)
    k.add(form, 'velvet', 0x161618)
    fx = bxp - 0.03
    bead_chain(k, [(fx - 0.004 * (1 - abs(t)), byp + 0.075 * t, z0 + 0.205 - 0.06 * (1 - t * t)) for t in np.linspace(-1.0, 1.0, 11)], 0.0042, 0xece4d6, 'gloss')
    bead_chain(k, [(fx - 0.006 * (1 - abs(t)), byp + 0.09 * t, z0 + 0.20 - 0.095 * (1 - t * t)) for t in np.linspace(-1.0, 1.0, 11)], 0.0055, 0x8a1e22, 'gloss')
    k.add(xf(lathe_axis('pendant', [(0.0, -0.012), (0.012, -0.004), (0.01, 0.006), (0.0, 0.01)], segments=16), M(fx - 0.006, byp, z0 + 0.095, 0, -90)), 'brass', 0xffffff)
    # brooches on a pad, bangles, watches on a pillow
    k.add(bx('pad', -0.18, -0.02, 0.32, 0.52, z0, z0 + 0.012, 0.005), 'velvet', 0x5a1a22)
    for i in range(6):
        bxx, byy = -0.15 + (i % 3) * 0.05, 0.36 + (i // 3) * 0.1
        k.add(lathe_axis('brooch', [(0.0, 0.0), (0.016, 0.0), (0.017, 0.004), (0.01, 0.009), (0.0, 0.01)], M(bxx, byy, z0 + 0.012), 16), 'brass', 0xffffff)
        k.add(lathe_axis('stone', [(0.0, 0.008), (0.007, 0.008), (0.0, 0.014)], M(bxx, byy, z0 + 0.012), 12), 'gloss', rnd.choice((0x8a1e22, 0x1e5a8a, 0x2a7a3a, 0xe8e2d0)))
    for i in range(4):
        k.add(xf(core.lathe('bangle', [(0.032, 0.0), (0.036, 0.0), (0.036, 0.008), (0.032, 0.008), (0.032, 0.0)], 32), M(0.02, 0.38 + i * 0.006, z0 + 0.03 + i * 0.002,
                                                                                                                          0, 75 - i * 6)),
              'brass' if i % 2 else 'gloss', rnd.choice((0x2a5a4a, 0x8a1e22, 0xe8dcc0)))
    k.add(soft_slab('pillow', 0.07, 0.26, 0.05, 0.02, 4, 8, 3, sag=0.006, matrix=M(0.12, -0.02 + 0.0, z0)), 'velvet', 0x3a2a1e)
    for i, yy in enumerate((-0.09, -0.03, 0.03, 0.09)):
        watch(k, (0.12, yy - 0.0, z0 + 0.05), 0, metal='steel' if i % 2 else 'brass', strap=(0x2a1a10, 0x1a1a1a, 0x5a3a20, 0x2a2a3a)[i])
    k.panel('askbea', 0.13, 0.08, draw_askbea)
    k.decal('askbea', (-hd - 0.0126, -hl / 2, CS_DECK - 0.22), u=(0, -1, 0), v=(0, 0, 1), lift=0.0)
    k.grime += [((-hd, 0.0, CS_H), 0.6, 0.3)]
    return [k.finish(angle=40.0, unbaked_order=['ns_glass'])]


# ------------------------------------------------------------------ the fitting booth and the three-way mirror (PROP)

BOOTH_W, BOOTH_D, BOOTH_H = 1.0, 1.0, 2.13
RAIL_Z = 2.06
STOOL_SEAT = 0.46
MIRROR_AT = (-1.05, 1.32)      # where the hero stands to look in the three-way mirror (booth frame), facing +X


def draw_limit(s, w, h):
    s.rect(0, 0, w, h, 0xf6f2e6, rough=0.85)
    hand_mid(s, 'LIMIT 4 ITEMS', w / 2, h * 0.66, 24, w - 18, 0x1a1a1a, width=0.11, seed=51)
    hand_mid(s, 'curtain sticks - pull, dont yank', w / 2, h * 0.28, 11, w - 16, 0x1b2f7a, width=0.08, seed=52)


def booth():
    """SM_NS_Thrift_Booth: a fitting booth of painted panels (its opening toward -X) with a chrome ring rail across the
    front and round the corner, a stool, a double hook with a hanger; beside it at +Y the three-way mirror on a plinth
    (center panel and two wings, [mirror] slots). SM_NS_Thrift_Booth_Curtain hangs on the rail."""
    k = Kit('thb', 'SM_NS_Thrift_Booth', TEXTURE_SIZES, dust=0.25, low_grime=0.15, groups=['body'])
    hw, hd = BOOTH_W / 2, BOOTH_D / 2
    t = 0.03
    # the back and side panels: framed, painted, a cap
    for (a, b, c_, d_) in ((hd - t, hd, -hw, hw), (-hd, hd, -hw - t, -hw), (-hd, hd, hw, hw + t)):
        k.add(bx('wall', a, b, c_, d_, 0.03, BOOTH_H, 0.004), 'paint', 0xc9cfc0, dir=2)
    for (xx, yy) in ((-hd, -hw - t), (-hd, hw), (hd - t, -hw - t), (hd - t, hw)):
        k.add(bx('post', xx - 0.01, xx + 0.04, yy - 0.005, yy + t + 0.005, 0.0, BOOTH_H + 0.02, 0.004), 'wood', 0x6a4a2e, dir=2)
    for (a, b, c_, d_) in ((hd - t - 0.01, hd + 0.01, -hw - t - 0.01, hw + t + 0.01), (-hd - 0.01, hd + 0.01, -hw - t - 0.02, -hw + 0.0),
                           (-hd - 0.01, hd + 0.01, hw, hw + t + 0.02)):
        k.add(bx('cap', a, b, c_, d_, BOOTH_H, BOOTH_H + 0.03, 0.006), 'wood', 0x6a4a2e, dir=0 if (b - a) > (d_ - c_) else 1)
        k.add(bx('base', a, b, c_, d_, 0.0, 0.09, 0.004), 'wood', 0x5a3a22, dir=0 if (b - a) > (d_ - c_) else 1)
    # the floor inside: a worn rubber mat
    k.add(bx('mat', -hd + 0.03, hd - t - 0.01, -hw + 0.01, hw - 0.01, 0.0, 0.008, 0.004), 'rubber', 0x2a2724)
    # the ring rail: across the front at RAIL_Z, bent round the corner at -Y back to the side panel, on two brackets
    rail = bend([(-hd - 0.04, hw + 0.02, RAIL_Z), (-hd - 0.04, -hw - 0.06, RAIL_Z), (-hd + 0.25, -hw - 0.06, RAIL_Z)], 0.12, 8)
    k.add(tube('rail', rail, 0.0125, 14), 'chrome', var=0.3)
    for (p, q) in (((-hd - 0.04, hw + 0.02), (-hd + 0.012, hw + 0.02)), ((-hd + 0.25, -hw - 0.06), (-hd + 0.25, -hw - t))):
        k.add(rod('rail_bracket', (p[0], p[1], RAIL_Z), (q[0], q[1], RAIL_Z), 0.008, 10), 'chrome', var=0.4)
        dn = (Vector((q[0] - p[0], q[1] - p[1], 0.0))).normalized()
        k.add(rod('flange', Vector((q[0], q[1], RAIL_Z)) - dn * 0.006, (q[0], q[1], RAIL_Z), 0.022, 16), 'chrome', var=0.4)
    # the stool (seat top STOOL_SEAT) in the back corner, the double hook with a hanger on the back panel
    sx_, sy_ = 0.22, -0.22
    k.add(xf(lathe_axis('stool_seat', [(0.0, STOOL_SEAT - 0.05), (0.16, STOOL_SEAT - 0.05), (0.17, STOOL_SEAT - 0.02), (0.16, STOOL_SEAT), (0.0, STOOL_SEAT + 0.004)],
                        segments=32), M(sx_, sy_, 0.0)), 'vinyl', 0x7a1418)
    k.add(xf(lathe_axis('stool_ring', [(0.0, STOOL_SEAT - 0.07), (0.15, STOOL_SEAT - 0.07), (0.15, STOOL_SEAT - 0.05), (0.0, STOOL_SEAT - 0.05)], segments=32),
             M(sx_, sy_, 0.0)), 'chrome', var=0.4)
    for a in (45, 135, 225, 315):
        ca, sa = math.cos(math.radians(a)), math.sin(math.radians(a))
        k.add(tube('stool_leg', [(sx_ + ca * 0.12, sy_ + sa * 0.12, STOOL_SEAT - 0.07), (sx_ + ca * 0.17, sy_ + sa * 0.17, 0.02)], 0.011, 10), 'chrome', var=0.6)
        k.add(xf(lathe_axis('stool_foot', [(0.0, 0.0), (0.014, 0.0), (0.012, 0.02), (0.0, 0.02)], segments=12), M(sx_ + ca * 0.17, sy_ + sa * 0.17, 0.0)), 'rubber', 0x111111)
    k.add(tube('stool_footring', [(sx_ + 0.145 * math.cos(a), sy_ + 0.145 * math.sin(a), 0.2) for a in np.linspace(0, 2 * math.pi, 32, endpoint=False)], 0.008, 8,
               closed=True), 'chrome', var=0.5)
    hx_ = hd - t
    k.add(bx('hook_plate', hx_ - 0.006, hx_, 0.17, 0.23, 1.62, 1.74, 0.006), 'brass', 0xffffff)
    for dz, L in ((0.0, 0.07), (0.05, 0.05)):
        k.add(tube('hook', bend([(hx_ - 0.006, 0.2, 1.68 + dz), (hx_ - L, 0.2, 1.68 + dz), (hx_ - L - 0.012, 0.2, 1.70 + dz)], 0.01), 0.004, 8), 'brass', 0xffffff)
    hg = wire_hanger()
    hg.data.transform(M(hx_ - 0.07, 0.2, 1.685, 90))
    k.add(hg, 'chrome', var=0.5)
    k.panel('limit', 0.2, 0.13, draw_limit)
    k.decal('limit', (-0.15, -hw - t - 0.0005, 1.45), u=(1, 0, 0), v=(0, 0, 1), lift=0.0)
    # the three-way mirror at +Y: a walnut plinth, a center panel facing -X, wings angled 32 degrees forward
    my = 1.32
    k.add(bx('plinth', -0.34, 0.12, my - 0.62, my + 0.62, 0.0, 0.10, 0.006), 'wood', 0x3a2618, dir=1)
    k.add(bx('plinth_top', -0.36, 0.14, my - 0.64, my + 0.64, 0.10, 0.125, 0.006), 'wood', 0x4a3020, dir=1)
    mz0, mz1 = 0.17, 1.98
    cw, ww = 0.62, 0.44
    ang = math.radians(32)
    eL, eR = Vector((0.0, my + cw / 2, 0.0)), Vector((0.0, my - cw / 2, 0.0))     # the center panel's left and right edges (seen from -X)
    panels = [(eL + Vector((-math.sin(ang) * ww, math.cos(ang) * ww, 0.0)), eL), (eL, eR), (eR, eR + Vector((-math.sin(ang) * ww, -math.cos(ang) * ww, 0.0)))]
    for (a, b) in panels:
        d = b - a
        L = d.length
        dn = d.normalized()
        n = dn.cross(Vector((0.0, 0.0, 1.0)))     # toward the viewer
        yaw = math.degrees(math.atan2(d.y, d.x))
        c = (a + b) / 2
        k.add(xf(parts.rbox('mback', (0.0, 0.0, (mz0 + mz1) / 2), (L, 0.02, mz1 - mz0), 0.004, 2), M(c.x - n.x * 0.014, c.y - n.y * 0.014, 0.0, yaw)),
              'wood', 0x3a2618, dir=2)
        for (u0_, u1_, z0_, z1_) in ((0.0, 0.05, mz0, mz1), (L - 0.05, L, mz0, mz1), (0.05, L - 0.05, mz0, mz0 + 0.05), (0.05, L - 0.05, mz1 - 0.05, mz1)):
            p = a + dn * ((u0_ + u1_) / 2) + n * 0.008
            k.add(xf(parts.rbox('mframe', (0.0, 0.0, 0.0), (u1_ - u0_, 0.024, z1_ - z0_), 0.005, 2), M(p.x, p.y, (z0_ + z1_) / 2, yaw)), 'wood', 0x3a2618,
                  dir=2 if (z1_ - z0_) > (u1_ - u0_) else 0)
        g0 = a + dn * 0.05 + n * 0.002
        g1 = b - dn * 0.05 + n * 0.002
        mq = quad('mirror', [(g0.x, g0.y, mz0 + 0.05), (g1.x, g1.y, mz0 + 0.05), (g1.x, g1.y, mz1 - 0.05), (g0.x, g0.y, mz1 - 0.05)])
        k.special(mq, UNB['ns_mirror'])
    # brass hinges between the panels, behind
    for e in (eL, eR):
        for hz in (0.5, 1.0, 1.6):
            k.add(rod('mhinge', (e.x + 0.012, e.y, hz), (e.x + 0.012, e.y, hz + 0.08), 0.006, 10), 'brass', 0xffffff)
    k.grime += [((-hd, -hw, 1.1), 0.25, 0.5)]
    obj = k.finish(angle=40.0, unbaked_order=['ns_mirror'])
    return [obj] + booth_curtain()


def booth_curtain():
    """SM_NS_Thrift_Booth_Curtain: the curtain drawn shut on the booth's front rail (origin at the rail's -Y end at the
    rings' top, the curtain hanging along +Y): the game scales it in Y (about 0.25 open, 1.0 shut) to draw it."""
    k = Kit('thbc', 'SM_NS_Thrift_Booth_Curtain', TEXTURE_SIZES, dust=0.0, low_grime=0.08, groups=['cloth'])
    k.cavity_amt = 0.3
    width, drop = 1.04, 1.96
    nf = 13
    rows, cols = 24, 104
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    grid = []
    rnd = random.Random(8)
    ph = [rnd.uniform(-0.3, 0.3) for _ in range(nf + 2)]
    amps = [rnd.uniform(0.6, 1.4) for _ in range(nf + 2)]
    P_noise = np.array([[0.0, 0.0, 0.0]])
    for j in range(rows + 1):
        z = -0.03 - drop * j / rows
        row = []
        for i in range(cols + 1):
            s = i / cols
            y = s * width
            f = s * nf
            fi = int(f) % len(ph)
            amp = 0.042 * amps[fi] * (1.0 - 0.3 * (j / rows))
            # folds wander a little as they fall; the hem kicks out where a fold meets the floor
            x = amp * math.sin(2 * math.pi * f + ph[fi] * (j / rows) + 0.35 * math.sin(3.1 * f + 2.0 * j / rows))
            if j == rows:
                z += 0.012 * math.sin(7.3 * f)
            row.append(bm.verts.new((x, y, z)))
        grid.append(row)
    for j in range(rows):
        for i in range(cols):
            f_ = bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
            for loop, (a, b) in zip(f_.loops, ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))):
                loop[uvl].uv = (a / cols, 1 - b / rows)
    cl = core.mesh_object('curtain', bm)
    # thickness: solidify toward +X
    mod = cl.modifiers.new('solid', 'SOLIDIFY')
    mod.thickness = 0.006
    mod.offset = 0.0
    core.select_only([cl])
    bpy.ops.object.modifier_apply(modifier=mod.name)
    cl.data.shade_smooth()
    # a hem at the bottom, rings at the top
    k.add(cl, 'velvet', 0x4a1a1e, group='cloth')
    rings = []
    for i in range(nf + 1):
        y = (i / nf) * width
        rings.append(xf(core.lathe('ring', [(0.016, -0.003), (0.019, 0.0), (0.016, 0.003), (0.013, 0.0), (0.016, -0.003)], 16),
                        M(0.0, y, 0.0, 0, 0, 90)))
    k.add(core.join(rings, 'rings'), 'chrome', 0xffffff, group='cloth', var=0.4)
    return [k.finish(angle=60.0)]


# ------------------------------------------------------------------ the shoe wall and the hat wall (PROP, on a wall)

SLAT = 0.0762


def slatwall(k, y0, y1, z0, z1, col=0xd9d1bf):
    """A slatwall panel on the wall plane x = 0, its face toward -X: lands between T-grooves every 3 in, aluminum edges."""
    n = int((z1 - z0) / SLAT)
    for i in range(n):
        za = z0 + i * SLAT
        k.add(bx('land', -0.019, 0.0, y0, y1, za + 0.006, za + SLAT - 0.006, 0.0015, 1), 'slatwall', col, dir=1)
        k.add(bx('groove', -0.006, 0.0, y0, y1, za - 0.006, za + 0.006, 0.0), 'slatwall', _tone(col, 0.4), dir=1)
    for (a, b, c_, d_) in ((y0 - 0.012, y0, z0, z0 + n * SLAT), (y1, y1 + 0.012, z0, z0 + n * SLAT), (y0 - 0.012, y1 + 0.012, z0 - 0.012, z0),
                           (y0 - 0.012, y1 + 0.012, z0 + n * SLAT, z0 + n * SLAT + 0.012)):
        k.add(bx('trim', -0.022, 0.0, a, b, c_, d_, 0.002), 'alu', 0xffffff, dir=1 if (b - a) > (d_ - c_) else 2)
    return z0 + n * SLAT


SHOE_ROWS = (0.62, 0.96, 1.30, 1.64, 1.98)
SHOE_COLS = (-0.96, -0.48, 0.0, 0.48, 0.96)


def shoe_points():
    """SH0..SH24: where a single shoe stands on the shoe wall (row by row from the bottom, -Y to +Y): (x, y, z, yaw) of the
    shoe's sole middle on its shelf; a sneaker (its own frame: toe toward +Y, origin at the ankle 8.5 cm above the
    ground) goes at z + 0.085, yaw 0, its outside toward the room."""
    return [(-0.11, y, z + 0.004, 0.0) for z in SHOE_ROWS for y in SHOE_COLS]


def shoe_wall():
    """SM_NS_Thrift_ShoeWall: an 8 ft slatwall section (origin on the floor at the wall plane, its face toward -X) with
    25 chrome-lipped shoe shelves in five rows, a painted base."""
    k = Kit('thw', 'SM_NS_Thrift_ShoeWall', TEXTURE_SIZES, dust=0.3, low_grime=0.2)
    W = 2.44
    top = slatwall(k, -W / 2, W / 2, 0.36, 2.40)
    k.add(bx('base', -0.03, 0.0, -W / 2 - 0.012, W / 2 + 0.012, 0.0, 0.34, 0.004), 'paint', 0x3a3a34)
    k.add(bx('base_shoe', -0.035, 0.0, -W / 2 - 0.012, W / 2 + 0.012, 0.0, 0.08, 0.003), 'lino', 0x1e1a16)
    for z in SHOE_ROWS:
        for y in SHOE_COLS:
            # a shelf: a plate 0.34 x 0.15 on two slatwall brackets, a lip at the front
            k.add(bx('shelf', -0.17, -0.02, y - 0.17, y + 0.17, z - 0.006, z, 0.002), 'powder', 0xe8e6e0)
            # (one bevel segment on the lip and the brackets, none on the hooks in the slot: the spec's 15k)
            k.add(bx('lip', -0.176, -0.164, y - 0.17, y + 0.17, z - 0.006, z + 0.02, 0.002, 1), 'chrome', var=0.3)
            for yb in (y - 0.12, y + 0.12):
                k.add(bx('bracket', -0.16, -0.006, yb - 0.004, yb + 0.004, z - 0.06, z - 0.006, 0.0015, 1), 'powder', 0xe8e6e0)
                k.add(bx('bracket_hook', -0.012, 0.0, yb - 0.004, yb + 0.004, z - 0.075, z - 0.04), 'powder', 0xe8e6e0)
    k.add(bx('cap', -0.06, 0.0, -W / 2 - 0.02, W / 2 + 0.02, top + 0.012, top + 0.05, 0.006), 'wood', 0x6a4a2e, dir=1)
    k.grime += [((-0.1, 0.0, 0.9), 0.9, 0.2)]
    return [k.finish(angle=40.0)]


HAT_ROWS = (1.30, 1.66, 2.02)
HAT_COLS = (-0.42, -0.14, 0.14, 0.42)


def hat_points():
    """HT0..HT11: where a hat sits on the hat wall (rows from the bottom, -Y to +Y): (x, y, z, yaw), a hat's origin (the
    top of the head inside it, wearables.py), its front toward -X for yaw -90 (a hat's own front is -Y)."""
    return [(-0.27, y, z - 0.005, -90.0) for z in HAT_ROWS for y in HAT_COLS]


def hat_wall():
    """SM_NS_Thrift_HatWall: a 4 ft slatwall section (origin on the floor at the wall plane, face toward -X) with 12 hat
    hooks in three rows (angled pegs ending in a rounded form), a small shelf at the bottom with a hand-lettered card."""
    k = Kit('thh', 'SM_NS_Thrift_HatWall', TEXTURE_SIZES, dust=0.3, low_grime=0.2)
    W = 1.22
    top = slatwall(k, -W / 2, W / 2, 0.92, 2.36)
    for z in HAT_ROWS:
        for y in HAT_COLS:
            # a slatwall hook: the clip, an angled arm, the rounded form a hat sits on
            k.add(bx('clip', -0.012, 0.0, y - 0.02, y + 0.02, z - 0.16, z - 0.12, 0.002), 'chrome', var=0.3)
            k.add(tube('arm', [(-0.01, y, z - 0.14), (-0.12, y, z - 0.11), (-0.22, y, z - 0.08)], 0.005, 8), 'chrome', var=0.3)
            k.add(xf(lathe_axis('form', [(0.0, -0.06), (0.03, -0.055), (0.05, -0.03), (0.055, 0.0), (0.045, 0.03), (0.0, 0.04)], segments=24),
                     M(-0.27, y, z - 0.045)), 'plastic', 0x1c1c1c, var=0.1)
    k.add(bx('shelf', -0.26, -0.02, -W / 2 + 0.05, W / 2 - 0.05, 0.92 + 0.02, 0.92 + 0.04, 0.004), 'wood', 0x6a4a2e, dir=1)
    for yb in (-W / 2 + 0.15, W / 2 - 0.15):
        k.add(bx('shelf_br', -0.22, -0.006, yb - 0.006, yb + 0.006, 0.80, 0.94, 0.002), 'powder', 0x1c1c1c)
    k.add(bx('cap', -0.06, 0.0, -W / 2 - 0.02, W / 2 + 0.02, top + 0.012, top + 0.05, 0.006), 'wood', 0x6a4a2e, dir=1)
    k.grime += [((-0.1, 0.0, 1.6), 0.6, 0.15)]
    return [k.finish(angle=40.0)]


# ------------------------------------------------------------------ the rack tags (PROP, hung from the ceiling)

def tag_picture():
    s = Sheet(1000.0, 330.0)
    sub_ = Sub(s, 0.0, 0.0, 1.0)
    sub_.rect(0, 0, 1000, 330, 0xf7f5ee)
    sub_.rect(0, 0, 1000, 40, 0x1f4a44)
    sub_.mid('TEES', 500, 190, 200, 900, 0x1f4a44, 'Black', tracking=1.1)
    return render_sheet(s, 'thg_tag_picture', 768, samples=8)


TAG_DROP = 0.45


def tags():
    """SM_NS_Thrift_Tags: a hanging category sign (origin at the ceiling grid where its two cables clip on; the sign 0.76 x
    0.25 m hangs TAG_DROP below): a coroplast face in an aluminum channel; both faces [screen] ns_screen_tag (UV 0..1
    across each, as read from that side) for core to draw TEES, JACKETS, PANTS, SHOES, HATS & GLASSES."""
    k = Kit('thg', 'SM_NS_Thrift_Tags', TEXTURE_SIZES, dust=0.2, low_grime=0.0)
    W, Hh = 0.76, 0.25
    z1, z0 = -TAG_DROP, -TAG_DROP - Hh
    for (a, b, c_, d_) in ((-W / 2 - 0.01, W / 2 + 0.01, z1, z1 + 0.012), (-W / 2 - 0.01, W / 2 + 0.01, z0 - 0.012, z0), (-W / 2 - 0.01, -W / 2, z0, z1),
                           (W / 2, W / 2 + 0.01, z0, z1)):
        k.add(bx('channel', -0.008, 0.008, a, b, c_, d_, 0.002), 'alu', 0xffffff, dir=1)
    k.add(bx('core', -0.0025, 0.0025, -W / 2, W / 2, z0, z1, 0.0), 'plastic', 0xf2f0ea, var=0.5)
    scr = image_mat('ns_screen_tag', tag_picture(), rough=0.55)
    for sgn in (-1, 1):
        x_ = sgn * 0.0027
        a, b = (Vector((x_, W / 2 - 0.005, z0 + 0.005)), Vector((x_, -W / 2 + 0.005, z0 + 0.005))) if sgn < 0 else \
            (Vector((x_, -W / 2 + 0.005, z0 + 0.005)), Vector((x_, W / 2 - 0.005, z0 + 0.005)))
        q = quad('face', [tuple(a), tuple(b), (b.x, b.y, z1 - 0.005), (a.x, a.y, z1 - 0.005)])
        k.special(q, scr)
    # a ceiling hanging kit (the spec's 1k): a spring clip on the grid's tee, 1.5 mm aircraft cable, a cable gripper on
    # an eye in the channel's top
    for y in (-W / 2 + 0.06, W / 2 - 0.06):
        k.add(bx('tbar_clip', -0.013, 0.013, y - 0.007, y + 0.007, -0.014, 0.0, 0.0015, 1), 'galv', 0xffffff)
        k.add(rod('cable', (0.0, y, -0.014), (0.0, y, z1 + 0.034), 0.00075, 6), 'steel', 0xffffff)
        k.add(xf(lathe_axis('gripper', [(0.0, 0.0), (0.0045, 0.0), (0.0048, 0.004), (0.0045, 0.022), (0.003, 0.026), (0.0, 0.026)], segments=10),
                 M(0.0, y, z1 + 0.01)), 'chrome', var=0.5)
        k.add(bx('eye', -0.0025, 0.0025, y - 0.006, y + 0.006, z1 + 0.004, z1 + 0.012, 0.0), 'alu', 0xffffff)
    return [k.finish(angle=40.0, unbaked_order=['ns_screen_tag'])]


# ------------------------------------------------------------------ the mannequin (PROP, P1)

def limb(name, pts, radii, n=20, flat=None):
    """A round (or flattened: flat = depth/width) tube through 3D points with a radius per point, parallel-transported
    frames, capped: limbs, necks, fingers."""
    P = [Vector(p) for p in pts]
    m = len(P)
    tans = []
    for i in range(m):
        a = P[max(i - 1, 0)]
        b = P[min(i + 1, m - 1)]
        tans.append((b - a).normalized())
    ref = Vector((1.0, 0.0, 0.0)) if abs(tans[0].x) < 0.9 else Vector((0.0, 1.0, 0.0))
    nrm = (ref - tans[0] * ref.dot(tans[0])).normalized()
    rings = []
    for i in range(m):
        if i > 0:
            nrm = tans[i - 1].rotation_difference(tans[i]) @ nrm
            nrm = (nrm - tans[i] * nrm.dot(tans[i])).normalized()
        bi = tans[i].cross(nrm)
        r = radii[i]
        f = flat[i] if isinstance(flat, (list, tuple)) else (flat or 1.0)
        rings.append([P[i] + nrm * (r * f * math.cos(2 * math.pi * k / n)) + bi * (r * math.sin(2 * math.pi * k / n)) for k in range(n)])
    # close the ends with a small dome
    obj = core.loft(name, rings, caps=True)
    obj.data.shade_smooth()
    return obj


def seamless(objs, name, voxel=0.005, target_tris=9000):
    """Parts fused into one closed skin, as a molded form is (a jointless mannequin): voxel-remeshed, relaxed out of
    the voxel steps, decimated to target_tris and smooth-shaded. Each part is subdivided first (Catmull-Clark, two levels),
    so the lathes' and lofts' rings don't survive the remesh as facets."""
    for o in objs:
        sub = o.modifiers.new('round', 'SUBSURF')
        sub.levels = sub.render_levels = 2
        core.select_only([o])
        bpy.ops.object.modifier_apply(modifier=sub.name)
    obj = core.join(objs, name)
    obj.data.remesh_voxel_size = voxel
    obj.data.remesh_voxel_adaptivity = 0.0
    core.select_only([obj])
    bpy.ops.object.voxel_remesh()
    sm = obj.modifiers.new('relax', 'SMOOTH')
    sm.factor = 0.5
    sm.iterations = 3
    core.select_only([obj])
    bpy.ops.object.modifier_apply(modifier=sm.name)
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    if tris > target_tris:
        dec = obj.modifiers.new('reduce', 'DECIMATE')
        dec.ratio = target_tris / tris
        core.select_only([obj])
        bpy.ops.object.modifier_apply(modifier=dec.name)
    obj.data.shade_smooth()
    return obj


def mannequin(stance='A'):
    """SM_NS_Thrift_Mannequin (stance A: standing square, arms easy at the sides) and SM_NS_Thrift_Mannequin_B (weight
    on the left leg, the right knee eased, the left hand on the hip): a jointless store mannequin, 1.86 m, matte white
    fiberglass chipped to the grey gelcoat, an egg head, on a round steel base with a calf rod. Front toward -X, origin
    on the floor under the base's middle. Its sizes (for the garments looks hangs on it): chest 0.98 m round, waist
    0.80, hips 0.98, inseam 0.80, shoulder width 0.42."""
    name = 'SM_NS_Thrift_Mannequin' if stance == 'A' else 'SM_NS_Thrift_Mannequin_B'
    k = Kit('thm' if stance == 'A' else 'thmb', name, TEXTURE_SIZES, dust=0.35, low_grime=0.0)
    k.cavity = 0.02
    B = 0.025      # the base's top
    WHITE = 0xe4e1da
    # the shoulders roll from the neck's foot over the deltoid (no shelf where they meet the arms)
    torso = loft_z('torso', [(B + 1.535, 0.062, 0.056), (B + 1.528, 0.10, 0.064), (B + 1.515, 0.14, 0.074), (B + 1.497, 0.172, 0.085),
                             (B + 1.475, 0.192, 0.096), (B + 1.445, 0.199, 0.106), (B + 1.40, 0.19, 0.112), (B + 1.32, 0.172, 0.115),
                             (B + 1.22, 0.152, 0.1), (B + 1.12, 0.143, 0.094), (B + 1.03, 0.158, 0.1), (B + 0.95, 0.172, 0.108), (B + 0.89, 0.16, 0.10),
                             (B + 0.86, 0.12, 0.08)], n=36, p=0.8, step=0.03)
    for v in torso.data.vertices:          # the chest a little forward, the seat a little back
        z = v.co.z - B
        v.co.x += -0.012 * math.exp(-((z - 1.33) / 0.08) ** 2) + 0.015 * math.exp(-((z - 0.93) / 0.06) ** 2) * (1 if v.co.x > 0 else 0)
    skin = [torso]      # every body part, fused into one seamless shell below (a jointless form has no seams)
    skin.append(limb('neck', [(0.0, 0.0, B + 1.48), (-0.005, 0.0, B + 1.64)], [0.056, 0.05], 20))
    head = lathe_axis('head', [(0.0, B + 1.615), (0.05, B + 1.62), (0.085, B + 1.66), (0.098, B + 1.73), (0.094, B + 1.79), (0.07, B + 1.84), (0.0, B + 1.862)],
                      segments=32)
    head.data.transform(Matrix.Translation((-0.01, 0.0, 0.0)) @ Matrix.Diagonal((1.12, 0.92, 1.0, 1.0)))
    skin.append(head)
    # legs: hip -> knee -> ankle; feet
    for s_ in (-1, 1):
        relaxed = stance == 'B' and s_ > 0
        hip = Vector((0.0, s_ * 0.092, B + 0.93))
        knee = Vector((-0.03 if relaxed else -0.012, s_ * (0.105 if relaxed else 0.098), B + 0.52))
        ankle = Vector((0.03 if relaxed else 0.0, s_ * (0.13 if relaxed else 0.10), B + (0.115 if relaxed else 0.10)))
        mid1 = hip.lerp(knee, 0.45) + Vector((-0.008, 0.0, 0.0))
        calf = knee.lerp(ankle, 0.35) + Vector((0.012, 0.0, 0.0))
        skin.append(limb('leg', [hip + Vector((0, 0, 0.06)), hip, mid1, knee, calf, ankle], [0.07, 0.085, 0.072, 0.052, 0.056, 0.036], 24))
        toe = ankle + Vector((-0.20, s_ * (0.035 if relaxed else 0.015), -0.075))
        heel = ankle + Vector((0.045, 0.0, -0.075))
        skin.append(limb('foot', [heel + Vector((0, 0, 0.03)), ankle + Vector((0.0, 0.0, -0.04)), ankle.lerp(toe, 0.55) + Vector((0, 0, -0.005)),
                                  toe + Vector((0, 0, 0.012))], [0.035, 0.042, 0.04, 0.022], 20, flat=[1.0, 0.75, 0.62, 0.55]))
    # arms: shoulder -> elbow -> wrist -> a mitten hand
    for s_ in (-1, 1):
        sh = Vector((0.0, s_ * 0.19, B + 1.455))
        if stance == 'B' and s_ < 0:      # the hand on the hip, the elbow out
            el = Vector((0.06, s_ * 0.33, B + 1.17))
            wr = Vector((0.02, s_ * 0.19, B + 1.03))
            hand_dir = Vector((-0.05, s_ * 0.03, -0.06))
        else:
            el = Vector((0.03, s_ * 0.235, B + 1.17))
            wr = Vector((-0.02, s_ * 0.255, B + 0.92))
            hand_dir = Vector((-0.02, 0.0, -0.16))
        skin.append(limb('arm', [sh + Vector((0, -s_ * 0.06, 0.01)), sh, sh.lerp(el, 0.5), el, el.lerp(wr, 0.5), wr], [0.058, 0.054, 0.045, 0.036, 0.034, 0.026], 20))
        hp = [wr, wr + hand_dir * 0.4, wr + hand_dir * 0.85, wr + hand_dir]
        skin.append(limb('hand', hp, [0.026, 0.038, 0.034, 0.015], 18, flat=[0.8, 0.42, 0.4, 0.45]))
        skin.append(limb('thumb', [wr + hand_dir * 0.2, wr + hand_dir * 0.45 + Vector((-0.025, 0.0, 0.0))], [0.014, 0.011], 12))
    k.add(seamless(skin, 'mannequin_skin', voxel=0.0045, target_tris=9000), 'paint', WHITE)
    # the base and the calf rod
    k.add(lathe_axis('base', [(0.0, 0.0), (0.2, 0.0), (0.205, 0.006), (0.2, B), (0.0, B)], segments=48), 'steel', 0xffffff)
    rod_y = 0.098 if stance == 'A' else -0.098
    k.add(rod('calf_rod', (0.05, rod_y, B), (0.03, rod_y, B + 0.26), 0.009, 12), 'chrome', var=0.5)
    k.add(lathe_axis('rod_flange', [(0.0, B), (0.03, B), (0.025, B + 0.012), (0.0, B + 0.014)], M(0.05, rod_y, 0.0), 20), 'chrome', var=0.5)
    k.grime += [((-0.1, 0.0, 1.2), 0.5, 0.15), ((0.0, 0.25, 0.95), 0.12, 0.4), ((0.0, -0.25, 0.95), 0.12, 0.4)]
    return [k.finish(angle=50.0)]


# ------------------------------------------------------------------ the misc wall (PROP, P1)

def draw_print(kind):
    def draw(s, w, h):
        rnd = random.Random(len(kind) * 31)
        if kind == 'sea':
            s.rect(0, 0, w, h, 0x9cb6c8)
            s.rect(0, 0, w, h * 0.42, 0x2f5a78)
            for i in range(30):
                s.rect(rnd.uniform(0, w), rnd.uniform(0, h * 0.42), rnd.uniform(20, 80), rnd.uniform(1.5, 4), 0xdfe8ee, alpha=0.6)
            s.poly([(w * 0.38, h * 0.38), (w * 0.62, h * 0.38), (w * 0.58, h * 0.31), (w * 0.42, h * 0.31)], 0x5a3a26)
            s.poly([(w * 0.5, h * 0.40), (w * 0.5, h * 0.8), (w * 0.66, h * 0.42)], 0xf2eee4)
            s.poly([(w * 0.49, h * 0.42), (w * 0.49, h * 0.72), (w * 0.38, h * 0.42)], 0xe8e2d4)
        elif kind == 'fruit':
            s.rect(0, 0, w, h, 0x3a2a1e)
            s.rect(0, 0, w, h * 0.3, 0x6a4a2e)
            for cx, cy, r, c in ((0.4, 0.42, 0.12, 0xb8321e), (0.58, 0.4, 0.1, 0xd88a1e), (0.5, 0.5, 0.09, 0x6a8a2e), (0.68, 0.46, 0.08, 0x8a2a5a)):
                s.circle(w * cx, h * cy, w * r, c, n=40)
                s.circle(w * cx - w * r * 0.3, h * cy + w * r * 0.3, w * r * 0.25, 0xf0e0c0, n=20, alpha=0.5)
        else:
            s.rect(0, 0, w, h, 0xc9d2c0)
            s.poly([(0, h * 0.3), (w * 0.3, h * 0.62), (w * 0.55, h * 0.4), (w * 0.8, h * 0.7), (w, h * 0.45), (w, 0), (0, 0)], 0x5a7a4a)
            s.poly([(0, 0), (w, 0), (w, h * 0.2), (0, h * 0.25)], 0x7a6a3a)
            s.circle(w * 0.78, h * 0.82, w * 0.06, 0xf2e2a0, n=30)
    return draw


FLYERS = [('CHOIR', 'practice thurs 7pm', 0xf2efe6), ('FOOD PANTRY', 'tues & thurs', 0xf6e27a), ('LOST CAT', 'grey, answers to Biscuit', 0xf2efe6),
          ('BAKE SALE', 'after 10:30 mass', 0xf4c6c6), ('ROOM FOR RENT', 'call 555-0187', 0xd8ecf2), ('MEN\'S GROUP', 'basement, mon 8pm', 0xf2efe6),
          ('PIANO LESSONS', 'all ages 555-0163', 0xe2f2d8)]


def flyer(title, sub, ground):
    def draw(s, w, h):
        rnd = random.Random(len(title))
        s.rect(0, 0, w, h, ground, rough=0.85)
        hand_mid(s, title, w / 2, h * 0.7, h * 0.18, w - 14, 0x1a1a1a, width=0.11, seed=len(title))
        hand_mid(s, sub, w / 2, h * 0.38, h * 0.1, w - 16, 0x1b2f7a, width=0.08, seed=len(sub))
        if 'call' in sub or '555' in sub:
            for i in range(7):
                s.rect(4 + i * (w - 8) / 7, 0, 1.0, h * 0.18, 0x8a8478, rough=0.8)
    return draw


def draw_bin(s, w, h):
    s.rect(0, 0, w, h, 0x2f4f3a, rough=0.6)
    s.mid('DONATIONS', w / 2, h * 0.62, h * 0.24, w * 0.9, 0xece4cc, 'Black', tracking=1.2)
    hand_mid(s, 'God bless - St. Agnes', w / 2, h * 0.25, h * 0.14, w * 0.85, 0xece4cc, width=0.1, seed=77)


def draw_asis(s, w, h):
    s.rect(0, 0, w, h, 0xf6f2e4, rough=0.85)
    hand_mid(s, 'AS IS', w / 2, h * 0.62, h * 0.36, w - 10, 0xb0161c, width=0.12, seed=81)
    hand_mid(s, 'works!', w / 2, h * 0.24, h * 0.2, w - 10, 0x1a1a1a, width=0.09, seed=82)


def misc():
    """SM_NS_Thrift_Misc: the misc wall (origin on the floor at the wall plane, the pieces toward -X, spanning Y -2.25 ..
    2.25): a bookcase of books with LPs in a crate, the furniture corner (a mustard armchair, an unlit floor lamp, a
    console CRT on its stand marked AS IS), three framed prints over it, St. Agnes' bulletin board of flyers, the
    donation bin (the thrift-donate use) with its slot."""
    k = Kit('thx', 'SM_NS_Thrift_Misc', TEXTURE_SIZES, dust=0.35, low_grime=0.12)
    rnd = random.Random(1987)
    # the bookcase: painted pine, five shelves of books
    by0, by1, bd, bh = -2.22, -1.22, 0.32, 1.85
    for (a, b) in ((by0, by0 + 0.02), (by1 - 0.02, by1)):
        k.add(bx('bc_side', -bd, 0.0, a, b, 0.0, bh, 0.003), 'paint', 0xd9d0bc, dir=2)
    k.add(bx('bc_back', -0.012, 0.0, by0, by1, 0.0, bh, 0.0), 'plywood', 0x9a7a52, dir=1)
    shelves = [0.06, 0.40, 0.74, 1.08, 1.42, bh - 0.02]
    for z in shelves:
        k.add(bx('bc_shelf', -bd, -0.012, by0 + 0.02, by1 - 0.02, z, z + 0.02, 0.002), 'paint', 0xd9d0bc, dir=1)
    k.add(bx('bc_kick', -bd, -bd + 0.02, by0 + 0.02, by1 - 0.02, 0.0, 0.06, 0.002), 'paint', 0xd9d0bc)
    book_cols = [0x6a2424, 0x24384a, 0x2a4a34, 0x8a6a2a, 0x3a3a3a, 0xc8b88a, 0x5a2a4a, 0x8a3a1a, 0x1a2a3a, 0xd8d0c0, 0x4a5a6a]
    for zi, z in enumerate(shelves[:-1]):
        y = by0 + 0.03
        z0 = z + 0.02
        while y < by1 - 0.05:
            if zi == 0:
                break
            if rnd.random() < 0.06:
                y += rnd.uniform(0.04, 0.12)
                continue
            t = rnd.uniform(0.018, 0.045)
            hgt = rnd.uniform(0.18, 0.29)
            dep = rnd.uniform(0.14, 0.22)
            if y + t > by1 - 0.03:
                break
            lean = 0.0 if rnd.random() > 0.12 else rnd.uniform(8, 18)
            m = M(-0.02 - dep / 2, y + t / 2, z0, 0, 0, -lean) if lean else M(-0.02 - dep / 2, y + t / 2, z0)
            k.add(xf(bx('book', -dep / 2, dep / 2, -t / 2, t / 2, 0.0, hgt, 0.003), m), 'cardboard', rnd.choice(book_cols), var=rnd.random())
            y += t + (0.01 if lean else 0.001)
    # the bottom shelf: LPs on edge
    y = by0 + 0.05
    while y < by1 - 0.06:
        k.add(xf(bx('lp', -0.31, -0.0, -0.002, 0.002, 0.0, 0.31, 0.0), M(-0.015, y, 0.082, 0, 0, rnd.uniform(-3, 3))), 'cardboard', rnd.choice(TINTS), var=rnd.random())
        y += 0.006
    # a crate of records on the floor
    cx0, cx1, cy0, cy1 = -0.42, -0.06, -1.10, -0.68
    for (a, b, c_, d_) in ((cx0, cx1, cy0, cy0 + 0.015), (cx0, cx1, cy1 - 0.015, cy1), (cx0, cx0 + 0.015, cy0, cy1), (cx1 - 0.015, cx1, cy0, cy1)):
        for zz in (0.02, 0.14, 0.26):
            k.add(bx('slat', a, b, c_, d_, zz, zz + 0.09, 0.002), 'plywood', 0xb08a5a, dir=0 if (b - a) > (d_ - c_) else 1)
    k.add(bx('crate_floor', cx0, cx1, cy0, cy1, 0.0, 0.02, 0.0), 'plywood', 0xb08a5a)
    x = cx0 + 0.04
    while x < cx1 - 0.04:
        k.add(xf(bx('lp', -0.002, 0.002, -0.155, 0.155, 0.0, 0.31, 0.0), M(x, (cy0 + cy1) / 2, 0.02, 0, rnd.uniform(-8, 14), 0)), 'cardboard', rnd.choice(TINTS),
              var=rnd.random())
        x += 0.008
    # the floor lamp (off), the armchair, the CRT on its stand
    lx, ly = -0.30, -0.52
    k.add(lathe_axis('lamp_base', [(0.0, 0.0), (0.14, 0.0), (0.14, 0.02), (0.05, 0.04), (0.0, 0.045)], M(lx, ly, 0.0), 32), 'brass', 0xffffff)
    k.add(rod('lamp_pole', (lx, ly, 0.04), (lx, ly, 1.42), 0.011, 12), 'brass', 0xffffff)
    shade = core.lathe('lamp_shade', [(0.20, 1.30), (0.15, 1.58)], 40)
    shade.data.transform(M(lx, ly, 0.0))
    mod = shade.modifiers.new('solid', 'SOLIDIFY')
    mod.thickness = 0.004
    core.select_only([shade])
    bpy.ops.object.modifier_apply(modifier=mod.name)
    k.add(shade, 'cloth', 0xe8dcb8)
    k.add(lathe_axis('lamp_socket', [(0.0, 1.36), (0.02, 1.36), (0.02, 1.42), (0.0, 1.42)], M(lx, ly, 0.0), 16), 'brass', 0xffffff)
    ac = M(-0.62, 0.02, 0.0, -18)
    mus = 0xb8862e
    k.add(xf(soft_slab('ch_seat', 0.62, 0.62, 0.14, 0.05, 8, 8, 3, sag=0.02, seed=3, noise_amt=0.004), ac @ M(0.0, 0.0, 0.30)), 'velvet', mus)
    k.add(xf(soft_slab('ch_back', 0.16, 0.66, 0.62, 0.06, 3, 8, 8, seed=4, noise_amt=0.004), ac @ M(0.30, 0.0, 0.38, 0, -10)), 'velvet', mus)
    for s_ in (-1, 1):
        k.add(xf(soft_slab('ch_arm', 0.66, 0.12, 0.26, 0.05, 8, 3, 4, seed=5 + s_), ac @ M(0.02, s_ * 0.36, 0.30)), 'velvet', mus)
        for xx in (-0.27, 0.27):
            k.add(xf(rod('ch_leg', (xx, s_ * 0.34, 0.0), (xx * 0.92, s_ * 0.32, 0.31), 0.018, 12), ac), 'wood', 0x5a3a20, dir=2)
    k.add(xf(bx('ch_rail', -0.32, 0.32, -0.36, 0.36, 0.28, 0.31, 0.006), ac), 'wood', 0x5a3a20)
    # the CRT on a low stand
    tx, ty = -0.30, 0.86
    k.add(bx('tv_stand', tx - 0.25, tx + 0.25, ty - 0.35, ty + 0.35, 0.0, 0.48, 0.008), 'laminate', 0x4a3424, var=0.2, dir=1)
    k.add(bx('tv_box', tx - 0.22, tx + 0.24, ty - 0.29, ty + 0.29, 0.48, 0.96, 0.03), 'plastic', 0x2a2724, var=0.3)
    scr = grid_plane('tv_screen', (tx - 0.226, ty + 0.235, 0.53), (0, -1, 0), (0, 0, 1), 0.47, 0.38, 6, 6,
                     disp=lambda s_, t_: -0.02 * (1 - (2 * s_ - 1) ** 2) * (1 - (2 * t_ - 1) ** 2))
    k.add(scr, 'glassbaked', 0x18201e)
    k.add(rod('ant1', (tx + 0.05, ty - 0.05, 0.96), (tx + 0.1, ty - 0.35, 1.38), 0.003, 6), 'chrome')
    k.add(rod('ant2', (tx + 0.05, ty + 0.05, 0.96), (tx + 0.05, ty + 0.3, 1.40), 0.003, 6), 'chrome')
    k.add(lathe_axis('ant_base', [(0.0, 0.96), (0.05, 0.96), (0.04, 0.99), (0.0, 1.0)], M(tx + 0.05, ty, 0.0), 20), 'plastic', 0x1a1a1a)
    k.panel('asis', 0.1, 0.07, draw_asis)
    k.decal('asis', (tx - 0.2215, ty - 0.2, 0.90), u=(0, -1, 0), v=(0, 0, 1), lift=0.0005)
    # three framed prints over the corner
    for i, (kind, py, pz, pw, ph) in enumerate((('sea', -0.55, 1.62, 0.5, 0.4), ('fruit', 0.1, 1.72, 0.36, 0.46), ('land', 0.68, 1.58, 0.56, 0.38))):
        k.add(bx('frame', -0.035, 0.0, py - pw / 2 - 0.04, py + pw / 2 + 0.04, pz - ph / 2 - 0.04, pz + ph / 2 + 0.04, 0.006), 'wood',
              (0x3a2414, 0xb8963e, 0x1a1a1a)[i], dir=1)
        k.panel(f'print_{kind}', pw, ph, draw_print(kind))
        k.decal(f'print_{kind}', (-0.0355, py, pz), u=(0, -1, 0), v=(0, 0, 1), lift=0.0)
    # the bulletin board: cork in an oak frame, flyers pinned up
    bb_y, bb_z, bw_, bh_ = 1.72, 1.52, 0.9, 0.6
    k.add(bx('bb_frame', -0.03, 0.0, bb_y - bw_ / 2 - 0.03, bb_y + bw_ / 2 + 0.03, bb_z - bh_ / 2 - 0.03, bb_z + bh_ / 2 + 0.03, 0.006), 'wood', OAK, dir=1)
    k.add(bx('bb_cork', -0.034, -0.01, bb_y - bw_ / 2, bb_y + bw_ / 2, bb_z - bh_ / 2, bb_z + bh_ / 2, 0.0), 'cork', 0xffffff)
    spots = [(-0.3, 0.14), (-0.06, 0.16), (0.2, 0.15), (0.34, -0.12), (-0.28, -0.13), (0.03, -0.1), (0.24, -0.02)]
    for i, ((t1, t2, g), (dy, dz)) in enumerate(zip(FLYERS, spots)):
        fw, fh = rnd.uniform(0.15, 0.2), rnd.uniform(0.2, 0.26)
        key = f'flyer{i}'
        k.panel(key, fw, fh, flyer(t1, t2, g))
        ang = math.radians(rnd.uniform(-5, 5))
        u = (0.0, -math.cos(ang), math.sin(ang))
        v = (0.0, math.sin(ang), math.cos(ang))
        k.decal(key, (-0.0345 - 0.0005 * i, bb_y + dy, bb_z + dz), u=u, v=v, lift=0.0, nu=2, nv=3,
                disp=lambda s_, t_: -0.004 * max(0.0, t_ - 0.75) * 4)
        k.add(lathe_axis('pin', [(0.0, 0.0), (0.006, 0.0), (0.005, 0.008), (0.0, 0.009)], M(-0.036 - 0.0005 * i, bb_y + dy, bb_z + dz + fh * 0.4, 0, -90), 12),
              'plastic', rnd.choice((0xc8321e, 0x2a6ab0, 0xe8c83a, 0x2a8a3a)))
    # the donation bin
    dx0, dx1, dy0, dy1 = -0.56, -0.02, 1.42, 2.02
    k.add(bx('bin', dx0, dx1, dy0, dy1, 0.0, 0.92, 0.01), 'paint', 0x2f4f3a, dir=2)
    k.add(bx('bin_lid', dx0 - 0.02, dx1 + 0.0, dy0 - 0.02, dy1 + 0.02, 0.92, 0.95, 0.008), 'paint', 0x2a4634)
    k.add(bx('chute', dx0 - 0.05, dx0 + 0.2, dy0 + 0.1, dy1 - 0.1, 0.70, 0.86, 0.01), 'galv', 0xffffff)
    k.add(bx('chute_slot', dx0 - 0.052, dx0 - 0.045, dy0 + 0.14, dy1 - 0.14, 0.76, 0.80, 0.003), 'paint', 0x0c0c0c)
    k.add(rod('chute_handle', (dx0 - 0.08, dy0 + 0.2, 0.83), (dx0 - 0.08, dy1 - 0.2, 0.83), 0.01, 10), 'chrome', var=0.6)
    k.panel('bin', 0.46, 0.3, draw_bin)
    k.decal('bin', (dx0 - 0.0005, (dy0 + dy1) / 2, 0.42), u=(0, -1, 0), v=(0, 0, 1), lift=0.0)
    k.grime += [((dx0, (dy0 + dy1) / 2, 0.8), 0.3, 0.6), ((-0.62, 0.02, 0.5), 0.5, 0.3)]
    return [k.finish(angle=45.0)]


# ================================================================== build

NAMES = {
    'front': ['SM_NS_Thrift_Front', 'SM_NS_Thrift_Front_Glass', 'SM_NS_Thrift_Front_GlowWarm'],
    'door': ['SM_NS_Thrift_Door', 'SM_NS_Thrift_Door_R'], 'counter': ['SM_NS_Thrift_Counter'], 'case': ['SM_NS_Thrift_Case'],
    'booth': ['SM_NS_Thrift_Booth', 'SM_NS_Thrift_Booth_Curtain'], 'shoewall': ['SM_NS_Thrift_ShoeWall'], 'hatwall': ['SM_NS_Thrift_HatWall'],
    'tags': ['SM_NS_Thrift_Tags'], 'mannequin': ['SM_NS_Thrift_Mannequin', 'SM_NS_Thrift_Mannequin_B'], 'misc': ['SM_NS_Thrift_Misc'],
    'shell': ['SM_NS_Thrift_Shell', 'SM_NS_Thrift_Shell_GlowCool'],
    'rack_round': ['SM_NS_Thrift_RackRound'], 'rack_straight': ['SM_NS_Thrift_RackStraight'], 'table': ['SM_NS_Thrift_Table'],
    'hung': ['SM_NS_Thrift_Hung_Tee', 'SM_NS_Thrift_Hung_Hoodie', 'SM_NS_Thrift_Hung_Jacket', 'SM_NS_Thrift_Hung_Pants'],
    'folded': ['SM_NS_Thrift_Folded_Tee', 'SM_NS_Thrift_Folded_Jeans', 'SM_NS_Thrift_Folded_Hoodie'],
}
BUILDERS = {
    'front': front, 'shell': shell, 'door': lambda: door_leaf(1) + door_leaf(-1), 'counter': counter, 'case': case, 'booth': booth,
    'shoewall': shoe_wall, 'hatwall': hat_wall, 'tags': tags, 'mannequin': lambda: mannequin('A') + mannequin('B'), 'misc': misc,
    'rack_round': rack_round, 'rack_straight': rack_straight, 'table': table,
    'hung': lambda: [o for kd in ('tee', 'hoodie', 'jacket', 'pants') for o in hung(kd)],
    'folded': lambda: [o for kd in ('tee', 'jeans', 'hoodie') for o in folded(kd)],
}
_ONLY = [s.strip() for s in os.environ.get('THRIFT_ONLY', '').split(',') if s.strip()]
WHICH = [k for k in NAMES if not _ONLY or k in _ONLY]
MESHES = [n for k in WHICH for n in NAMES[k]]


def build():
    core.reset()
    random.seed(220)
    make_unbaked()
    out = []
    for key in WHICH:
        out += BUILDERS[key]()
    for o in out:
        o.data.name = o.name
    return out


def assemble(objs):
    return assemble_core(objs, MESHES)


# ================================================================== review

# Thrift-store tints for the stand-in garments (sRGB): what a week's racks look like.
TINTS = [0x2b3a55, 0x8a8d90, 0x1e1e20, 0xe8e6e0, 0x6a2430, 0x5b5e3a, 0xb98c2e, 0x4a6a8c, 0x5a3a26, 0x2f4a3a, 0xa83232, 0xd8cfb8,
         0x2e6b6b, 0xc98a96, 0x3d3a48, 0x7a5c3e]


def tint_rgb(hexc):
    t, g = core.hex_linear(hexc), core.hex_linear(CLOTH_GREY)
    return tuple(min(t[i] / g[i], 1.6) for i in range(3))


def _find(objs, name):
    o = next((o for o in objs if o.name == name), None)
    return o if o is not None else import_glb(name)


# The looks track's garment statics (spec 4.7: same PROP frame, the hook's inside top at the origin), which ThriftStage
# hangs in place of the stand-ins once they're exported; the review dresses the racks with them when they exist.
LOOKS_RACK = {'Tee': ['SM_NS_Rack_Tee'], 'Hoodie': ['SM_NS_Rack_Hoodie'], 'Jacket': ['SM_NS_Rack_Bomber'],
              'Pants': ['SM_NS_Rack_Jeans', 'SM_NS_Rack_Chinos']}
LOOKS_UNTINTED = {'SM_NS_Rack_Jeans'}      # the jeans bake their own indigo (tinted white in the game)
_LOOKS = {}


def _looks(name):
    """A looks garment static imported once per run (hidden: dress copies of it), or None while it isn't exported."""
    if name not in _LOOKS:
        o = import_glb(name)
        if o is not None:
            o.hide_render = True
        _LOOKS[name] = o
    o = _LOOKS[name]
    try:
        return o if o is not None and o.name else None
    except ReferenceError:
        _LOOKS.pop(name, None)
        return None


def dress_rack(objs, rack, pts, seed, kinds, gaps=0.15, base=None):
    """Tinted garments on a rack's hanger points (the review only): the looks track's statics where they're exported
    (as ThriftStage hangs them), this module's stand-ins for the rest."""
    rnd = random.Random(seed)
    src = {}
    for kd in kinds:
        found = [o for o in (_looks(n) for n in LOOKS_RACK.get(kd, [])) if o is not None]
        src[kd] = found or [s for s in [_find(objs, f'SM_NS_Thrift_Hung_{kd}')] if s is not None]
    out = []
    base = base or Matrix()
    i = 0
    while i < len(pts):
        if rnd.random() < gaps:
            i += rnd.randint(1, 2)
            continue
        kd = rnd.choice(kinds)
        if not src[kd]:
            i += 1
            continue
        s = rnd.choice(src[kd])
        x, y, z, yaw = pts[i]
        m = base @ M(x, y, z, yaw + rnd.uniform(-6, 6), 0.0, rnd.uniform(-2, 2))
        rgb = (1.0, 1.0, 1.0) if s.name[3:] in LOOKS_UNTINTED else tint_rgb(rnd.choice(TINTS))
        out.append(tinted_copy(s, rgb, 0, m))
        i += rnd.choice((1, 1, 2, 2, 3)) if kd in ('Jacket', 'Hoodie') else rnd.choice((1, 1, 2))
    return out


def dress_shoes(objs, base, seed=4):
    """Sneakers from an earlier build (SM_Sneaker_L, tinted) on the shoe wall's shelves, gaps here and there."""
    src = _find(objs, 'SM_Sneaker_L')
    out = []
    if src is None:
        return out
    rnd = random.Random(seed)
    for (x, y, z, yaw) in shoe_points():
        if rnd.random() < 0.2:
            continue
        d = tinted_copy(src, tint_rgb(rnd.choice(TINTS)), 0, base @ M(x, y + rnd.uniform(-0.03, 0.03), z + 0.085, yaw + rnd.uniform(-6, 6)))
        out.append(d)
    src.hide_render = True
    return out


def dress_hats(objs, base, seed=6):
    srcs = [s for s in (_find(objs, n) for n in ('SM_Hat_Cap', 'SM_Hat_Beanie', 'SM_Hat_Bucket')) if s is not None]
    out = []
    if not srcs:
        return out
    rnd = random.Random(seed)
    for (x, y, z, yaw) in hat_points():
        if rnd.random() < 0.15:
            continue
        s = rnd.choice(srcs)
        out.append(tinted_copy(s, tint_rgb(rnd.choice(TINTS)), 0, base @ M(x, y, z, yaw + rnd.uniform(-8, 8))))
    for s in srcs:
        s.hide_render = True
    return out


def dress_table(objs, seed, base=None):
    rnd = random.Random(seed)
    src = [_find(objs, f'SM_NS_Thrift_Folded_{kd}') for kd in ('Tee', 'Tee', 'Tee', 'Hoodie')]
    out = []
    base = base or Matrix()
    for x, y, z in TABLE_STACKS:
        s = rnd.choice(src)
        if s is None:
            continue
        out.append(tinted_copy(s, tint_rgb(rnd.choice(TINTS)), 0, base @ M(x, y, z, rnd.uniform(-8, 8))))
    return out


PROP_SHOTS = {
    'SM_NS_Thrift_RackRound': [((-2.6, -1.7, 1.75), (0.0, 0.0, 0.95), 32), ((-0.75, -0.55, 0.42), (0.12, 0.05, 0.22), 40),
                               ((-1.25, 0.85, 1.75), (0.0, 0.0, 1.2), 35)],
    'SM_NS_Thrift_RackStraight': [((-2.7, -1.9, 1.7), (0.0, 0.0, 0.95), 32), ((-0.9, -1.45, 0.5), (0.0, -0.85, 0.2), 40),
                                  ((-1.3, 1.6, 1.9), (0.0, 0.3, 1.1), 35)],
    'SM_NS_Thrift_Table': [((-2.3, -1.5, 1.7), (0.0, 0.0, 0.62), 32), ((-0.75, 0.15, 1.3), (-0.25, 0.42, 0.74), 40),
                           ((-1.2, 1.5, 0.55), (0.0, 0.7, 0.35), 35)],
    'SM_NS_Thrift_Hung_Tee': [((-1.3, -0.5, 1.35), (0.0, 0.0, 1.2), 40), ((0.3, -1.3, 1.5), (0.0, 0.0, 1.2), 40)],
    'SM_NS_Thrift_Hung_Hoodie': [((-1.3, -0.5, 1.35), (0.0, 0.0, 1.2), 40), ((0.6, 1.2, 1.5), (0.0, 0.0, 1.2), 40)],
    'SM_NS_Thrift_Hung_Jacket': [((-1.3, -0.5, 1.35), (0.0, 0.0, 1.2), 40), ((0.3, -1.3, 1.5), (0.0, 0.0, 1.2), 40)],
    'SM_NS_Thrift_Hung_Pants': [((-1.3, -0.5, 1.35), (0.0, 0.0, 1.2), 40), ((0.3, -1.3, 1.5), (0.0, 0.0, 1.2), 40)],
    'SM_NS_Thrift_Folded_Tee': [((-0.7, -0.4, 0.55), (0.0, 0.0, 0.05), 45)],
    'SM_NS_Thrift_Folded_Jeans': [((-0.7, -0.4, 0.55), (0.0, 0.0, 0.07), 45)],
    'SM_NS_Thrift_Folded_Hoodie': [((-0.7, -0.4, 0.6), (0.0, 0.0, 0.09), 45)],
    'SM_NS_Thrift_Door': [((-2.1, 1.3, 1.35), (0.0, 0.45, 1.1), 35), ((1.8, 1.2, 1.4), (0.0, 0.45, 1.1), 35), ((-0.55, 0.6, 1.2), (0.0, 0.8, 1.1), 40)],
    'SM_NS_Thrift_Door_R': [((-2.1, -1.3, 1.35), (0.0, -0.45, 1.1), 35)],
    'SM_NS_Thrift_Counter': [((-2.4, -1.3, 1.7), (0.0, 0.0, 0.8), 32), ((-0.85, 0.25, 1.45), (0.0, 0.0, 1.05), 38), ((1.7, 1.2, 1.6), (0.0, 0.0, 0.8), 32)],
    'SM_NS_Thrift_Case': [((-1.9, -1.0, 1.5), (0.0, 0.0, 0.72), 32), ((-0.7, 0.05, 1.45), (-0.05, 0.0, 0.66), 40), ((1.3, 0.8, 1.3), (0.0, 0.0, 0.72), 35)],
    'SM_NS_Thrift_Booth': [((-3.3, -0.7, 1.7), (0.0, 0.55, 1.1), 26), ((-1.4, 0.15, 1.45), (0.3, -0.1, 0.9), 30), ((-1.9, 1.25, 1.45), (0.0, 1.32, 1.1), 32)],
    'SM_NS_Thrift_Booth_Curtain': [((-1.6, 0.5, 1.0), (0.0, 0.5, 1.0), 35)],
    'SM_NS_Thrift_ShoeWall': [((-3.0, -1.1, 1.4), (0.0, 0.0, 1.2), 30), ((-0.8, -0.55, 1.15), (0.0, -0.45, 0.98), 38)],
    'SM_NS_Thrift_HatWall': [((-2.2, -0.7, 1.6), (0.0, 0.0, 1.6), 32), ((-0.75, 0.35, 1.75), (0.0, 0.1, 1.6), 38)],
    'SM_NS_Thrift_Tags': [((-1.7, -0.7, 2.45), (0.0, 0.0, 2.72), 35)],
    'SM_NS_Thrift_Mannequin': [((-2.6, -1.0, 1.3), (0.0, 0.0, 0.95), 32), ((-0.9, 0.6, 1.7), (0.0, 0.0, 1.45), 35)],
    'SM_NS_Thrift_Mannequin_B': [((-2.6, -1.0, 1.3), (0.0, 0.0, 0.95), 32), ((1.6, -1.6, 1.4), (0.0, 0.0, 0.95), 32)],
    'SM_NS_Thrift_Misc': [((-4.2, -0.6, 1.6), (0.0, 0.0, 1.0), 26), ((-1.4, -1.2, 1.3), (0.0, -1.6, 0.9), 32), ((-1.5, 1.0, 1.5), (0.0, 1.0, 1.3), 30)],
}
CURTAIN_AT = (-BOOTH_D / 2 - 0.04, -0.50, RAIL_Z)
HUNG_AT = Matrix.Translation((0.0, 0.0, 1.6))


# Where the review stands the props (and what ThriftStage might do): (mesh, stage X, stage Y, stage yaw F).
LAYOUT = [
    ('SM_NS_Thrift_Counter', -2700, 4420, 180), ('SM_NS_Thrift_Case', -2700, 4267, 180),
    ('SM_NS_Thrift_HatWall', -2460, 3890, 180), ('SM_NS_Thrift_ShoeWall', -2592, 3650, 90),
    ('SM_NS_Thrift_RackRound', -3330, 4610, 0), ('SM_NS_Thrift_RackRound', -2850, 4740, 30),
    ('SM_NS_Thrift_Table', -3360, 4250, 90), ('SM_NS_Thrift_RackStraight', -2800, 3990, 0),
    ('SM_NS_Thrift_RackStraight', -3300, 3990, 0), ('SM_NS_Thrift_Booth', -3450, 3705, 90),
    ('SM_NS_Thrift_Misc', -3640, 4200, 0), ('SM_NS_Thrift_Mannequin', -2680, 4892, 90, 50), ('SM_NS_Thrift_Mannequin_B', -3440, 4892, 100, 50),
    ('SM_NS_Thrift_Door', -2962, 4830, 90), ('SM_NS_Thrift_Door_R', -3138, 4830, 90),
]
TAGS_AT = [(-3330, 4610, 0), (-2800, 3990, 0), (-3300, 3990, 0), (-2592, 3880, 90), (-2520, 3890, 0)]


def S2B(X, Y, F, z=0.0):
    """A PROP's matrix in the SITE frame from its stage position (cm) and yaw F (spec 0.4: Blender yaw -F)."""
    return M(-X / 100.0, Y / 100.0, z, -F)


def store_review(objs):
    """art/review/ns_thrift_store.jpg: the front from the sidewalk by night and by day, the recess, the sign, and the
    room from the door and from the back, with the props stood where LAYOUT puts them."""
    sc = bpy.context.scene
    by = {o.name: o for o in objs}
    temp = []
    saved_mw = {o.name: (o.matrix_world.copy(), o.hide_render) for o in objs}
    glow_cols = {'GlowWarm': (kelvin(2800), 6.0), 'GlowCool': (kelvin(4100), 9.0)}
    swaps = swap_for_review(objs, glow_cols)
    street = StreetMock(FRONT_Y, [(OX0, OX1, -0.31, 4.0)], (OX0 - 9.0, OX1 + 9.0), top=8.5)
    try:
        used = {}
        for row in LAYOUT:
            name, X, Y, F = row[:4]
            Z = row[4] / 100.0 if len(row) > 4 else 0.0
            src = by.get(name)
            if src is None:
                continue
            o = src if name not in used else src.copy()
            if name in used:
                core.link(o)
                temp.append(o)
            used[name] = True
            o.matrix_world = S2B(X, Y, F, Z)
            o.hide_render = False
            if name == 'SM_NS_Thrift_RackRound':
                temp += dress_rack(objs, name, round_rack_points(), int(-X) % 7, ['Tee', 'Tee', 'Hoodie', 'Tee'], base=o.matrix_world)
            elif name == 'SM_NS_Thrift_RackStraight':
                temp += dress_rack(objs, name, straight_rack_points(), int(-X) % 11, ['Jacket', 'Hoodie', 'Jacket'] if X > -3000 else ['Pants'],
                                   base=o.matrix_world)
            elif name == 'SM_NS_Thrift_Table':
                temp += dress_table(objs, 9, base=o.matrix_world)
            elif name == 'SM_NS_Thrift_ShoeWall':
                temp += dress_shoes(objs, o.matrix_world)
            elif name == 'SM_NS_Thrift_HatWall':
                temp += dress_hats(objs, o.matrix_world)
        if 'SM_NS_Thrift_Booth_Curtain' in by and 'SM_NS_Thrift_Booth' in by:      # drawn open, gathered at the rail's end
            c = by['SM_NS_Thrift_Booth_Curtain']
            c.matrix_world = by['SM_NS_Thrift_Booth'].matrix_world @ Matrix.Translation(CURTAIN_AT) @ Matrix.Diagonal((1.0, 0.28, 1.0, 1.0))
            c.hide_render = False
        if 'SM_NS_Thrift_Tags' in by:
            for i, (X, Y, F) in enumerate(TAGS_AT):
                o = by['SM_NS_Thrift_Tags'] if i == 0 else by['SM_NS_Thrift_Tags'].copy()
                if i:
                    core.link(o)
                    temp.append(o)
                o.matrix_world = S2B(X, Y, F, CZ)
                o.hide_render = False
        for o in objs:
            if o.name.startswith('SM_NS_Thrift_Hung_') or o.name.startswith('SM_NS_Thrift_Folded_'):
                o.hide_render = True
        temp.append(scale_figure((30.2, 51.4, 0.0), 160))
        temp.append(scale_figure((29.4, 46.0, 0.0), -20))
        # the room's light: ten troffers (4100 K), the window spots, the recess can, the goosenecks on the sign
        inside = []
        for tx, ty in TROFFERS:
            inside.append(area_light('rv_troffer', (tx, ty, CZ - 0.02), 0.55, 85.0, kelvin(4100), size_y=1.15))
        for x0, x1 in ((IX0 + 0.3, DX0 - 0.5), (DX1 + 0.5, IX1 - 0.3)):
            for sx in np.linspace(x0 + 0.5, x1 - 0.5, 3):
                inside.append(spot_light('rv_wspot', (sx, 48.99, CZ - 0.2), (sx, 49.4, PLAT_Z + 0.2), 60.0, kelvin(3000), 40, 0.5, 0.03))
        inside.append(spot_light('rv_can', (30.5, 48.95, 2.9), (30.5, 48.95, 0.0), 35.0, kelvin(2800), 100, 0.6, 0.05))
        for gx in (27.0, 30.5, 34.0):
            inside.append(spot_light('rv_gn', (gx, FRONT_Y + 0.5, 4.0), (gx, FRONT_Y, 3.6), 420.0, kelvin(2800), 100, 0.5, 0.05))
        temp += inside
        street.lamp((25.5, 53.4, 7.6), (25.5, 51.5, 0.0), 4000, 3500.0)
        street.lamp((36.5, 53.4, 7.6), (36.5, 51.5, 0.0), 4000, 3500.0)
        cd = bpy.data.cameras.new('rv_cam')
        cd.clip_start = 0.02
        cd.clip_end = 300.0
        cam = core.link(bpy.data.objects.new('rv_cam', cd))
        sc.camera = cam
        temp.append(cam)
        tmp_dir = os.path.join(core.BUILD_DIR, 'review', 'ns_thrift_store')
        os.makedirs(tmp_dir, exist_ok=True)
        paths = []
        street.night()
        paths.append(shot(cam, os.path.join(tmp_dir, 'night_front.png'), (30.5, 57.2, 1.7), (30.5, 49.5, 2.0), 20, exposure=1.0))
        paths.append(shot(cam, os.path.join(tmp_dir, 'night_angle.png'), (22.2, 53.0, 1.65), (31.0, 49.3, 2.2), 24, exposure=1.0))
        paths.append(shot(cam, os.path.join(tmp_dir, 'night_sign.png'), (26.6, 51.8, 2.5), (29.6, 49.5, 3.55), 30, exposure=1.0))
        street.day()
        for L in inside:
            L.data.energy *= 0.8
        paths.append(shot(cam, os.path.join(tmp_dir, 'day_front.png'), (30.5, 57.2, 1.7), (30.5, 49.5, 2.0), 20, exposure=-0.3))
        paths.append(shot(cam, os.path.join(tmp_dir, 'day_recess.png'), (30.5, 51.3, 1.6), (30.5, 48.7, 0.9), 26, exposure=-0.3))
        paths.append(shot(cam, os.path.join(tmp_dir, 'day_angle.png'), (38.8, 52.6, 1.65), (30.0, 49.3, 2.0), 24, exposure=-0.3))
        street.night()
        paths.append(shot(cam, os.path.join(tmp_dir, 'in_door.png'), (30.5, 47.9, 1.65), (30.0, 37.0, 1.2), 16, exposure=0.6))
        paths.append(shot(cam, os.path.join(tmp_dir, 'in_back.png'), (35.6, 37.4, 1.75), (28.5, 47.8, 1.3), 16, exposure=0.6))
        paths.append(shot(cam, os.path.join(tmp_dir, 'in_counter.png'), (29.9, 44.6, 1.6), (26.9, 42.8, 1.0), 24, exposure=0.6))
        fig = scale_figure((35.82, 38.15, 0.0), 90)
        temp.append(fig)
        paths.append(shot(cam, os.path.join(tmp_dir, 'in_fitting.png'), (35.2, 40.1, 1.75), (35.85, 37.1, 1.15), 26, exposure=0.6))
        paths.append(shot(cam, os.path.join(tmp_dir, 'in_walls.png'), (29.6, 40.8, 1.7), (25.2, 37.4, 1.3), 22, exposure=0.6))
        paths.append(shot(cam, os.path.join(tmp_dir, 'in_misc.png'), (31.4, 40.6, 1.6), (36.3, 42.2, 1.1), 22, exposure=0.6))
        compose('ns_thrift_store', paths, 3)
    finally:
        restore_swaps(swaps)
        street.close()
        for o in temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        for n, (mw, hid) in saved_mw.items():
            ob = bpy.data.objects.get(n)
            if ob is not None:
                ob.matrix_world, ob.hide_render = mw, hid
        sc.camera = None
        sc.view_settings.exposure = 0.0


def pose_for_review(objs):
    """Close looks (art/review/ns_thrift_props.jpg), the store (ns_thrift_store.jpg, when the site meshes are built), then
    the standard sheet of the props in a row."""
    global REVIEW_VIEWS
    names = {o.name for o in objs}
    if os.environ.get('THRIFT_REVIEW', '1') != '0' and names & {'SM_NS_Thrift_Front', 'SM_NS_Thrift_Shell'}:
        store_review(objs)
    place = {n: HUNG_AT for n in names if '_Hung_' in n}
    place['SM_NS_Thrift_Booth_Curtain'] = Matrix.Translation(CURTAIN_AT)
    place['SM_NS_Thrift_Tags'] = Matrix.Translation((0.0, 0.0, CZ))
    wall = {n: None for n in names if '_Hung_' in n or '_Folded_' in n}
    wall.update({'SM_NS_Thrift_Misc': 0.0, 'SM_NS_Thrift_ShoeWall': 0.0, 'SM_NS_Thrift_HatWall': 0.0, 'SM_NS_Thrift_Booth': 0.55, 'SM_NS_Thrift_Tags': None,
                 'SM_NS_Thrift_Door': None, 'SM_NS_Thrift_Door_R': None, 'SM_NS_Thrift_Booth_Curtain': None})
    if os.environ.get('THRIFT_REVIEW', '1') != '0':
        def dress(n):
            if n == 'SM_NS_Thrift_RackRound':
                return dress_rack(objs, n, round_rack_points(), 3, ['Tee', 'Tee', 'Hoodie', 'Tee'])
            if n == 'SM_NS_Thrift_RackStraight':
                return dress_rack(objs, n, straight_rack_points(), 5, ['Jacket', 'Jacket', 'Hoodie', 'Pants'])
            if n == 'SM_NS_Thrift_Table':
                return dress_table(objs, 7)
            if n == 'SM_NS_Thrift_ShoeWall':
                return dress_shoes(objs, Matrix())
            if n == 'SM_NS_Thrift_HatWall':
                return dress_hats(objs, Matrix())
            if n == 'SM_NS_Thrift_Booth':
                c = _find(objs, 'SM_NS_Thrift_Booth_Curtain')
                if c is not None:
                    d = c.copy()
                    core.link(d)
                    d.matrix_world = Matrix.Translation(CURTAIN_AT) @ Matrix.Diagonal((1.0, 0.3, 1.0, 1.0))
                    d.hide_render = False
                    return [d]
            return []
        detail_review(objs, PROP_SHOTS, 'ns_thrift_props', MODULE, dress=dress, place=place, wall=wall)
    # the standard sheet: the props in a row (the site meshes hidden)
    props = [o for o in objs if o.name in PROP_SHOTS]
    if not props:
        REVIEW_VIEWS = [('front', 0, 8, 0.6, (30.5, 49.5, 2.0)), ('angle', -40, 12, 0.6, (30.5, 49.5, 2.0)),
                        ('detail', -10, 10, 0.25, (30.5, 49.0, 1.4))]
        return
    for o in objs:
        if o not in props:
            o.hide_render = True
    objs[:] = props
    REVIEW_VIEWS = grid_layout(props, place)
