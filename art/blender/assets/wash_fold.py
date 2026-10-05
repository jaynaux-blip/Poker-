"""SM_WF_*: the Wash & Fold's interior kit (the laundromat at the foot of the brick tenement on Fifth Street, Northside,
across the street from 1812; Dee runs her Tue/Thu/Sat game in its back room). The game makes the laundromat enterable
and places these by name, with engine-shape fallbacks.

Conventions for every mesh: meters in Blender (centimeters in Unreal), Z up, the FRONT toward Blender -Y (Unreal +Y
after the importer's Y mirror; BlenderFacing() turns it like any other prop). Floor props have their origin on the
floor under the middle of their footprint (a machine's cabinet; doors, handles and knobs stand proud of it toward -Y).
Wall pieces (SM_WF_WallDressing, SM_WF_TV) have theirs on the floor, centered under the piece, ON THE WALL PLANE
(Y 0 = the wall's face; everything stands off it toward -Y), so they go straight against a wall. Printed text reads
left to right from the front (it is laid out in Blender's view, which the import keeps); faces read from the back or a
side are laid out from there.

Material slots: each mesh bakes a body slot (every hard-surface finish in one texture set, picked per face by an
attribute) and, where it has print, a print slot (an atlas of its labels and signs, emissive where a display or a lit
sign glows: the washer's price 6, card LCD 3, the dryers' counters 5, the changer's marquee 4 and arrows 5, the soap
vend's header 4 and tube 8, the counter's displays 3). The slots the game drives are left unbaked and named for it
(the same name in every mesh that has one):
  WF_Glass      door and window glass: a dark tint at alpha 0.2 (glTF BLEND): the washer's bowl, the dryers' doors,
                the soap vend's window.
  WF_Plastic    the clear bags of the counter's wash-and-fold orders: milky, alpha 0.42 (BLEND).
  WF_DrumGlow   the inside of each dryer drum and its ribs: perforated steel as an emissive texture (the base color is
                the same steel, darker). Idle it glows at 0.3, so the drums read dark behind the glass; the game turns
                it up (~1.5-2) to light a running drum. UV: u round the drum, v from the mouth (0) to the middle of
                the back plate (0.88); the ribs above 0.9.
  WF_Number     the machine-number stickers (the washer's 07 on its front and console, the dryer stack's 31 over 32):
                discs UV-mapped into T_WF_Numbers, a strip of stickers 01..32 left to right (number n spans u
                (n-1)/32..n/32, v 0..1; the texture repeats). The meshes show their default numbers as built; to
                number a row of machines, offset the material's U by k/32 per placement (the washer then reads 07+k,
                the stack 31+k over 32+k, wrapping past 32 to 01).
  WF_TVScreen   the TV's picture, UV 0..1 across the screen as seen (u to the right, v up); it shows a late-night poker
                broadcast at emissive strength 2.5 until the game puts its own picture there.

The meshes (W x D x H in meters, front toward -Y):
- SM_WF_Washer (0.69 x 0.81 x 1.10): a stainless front-load commercial washer. A porthole door (brushed ring, a glass
  bowl reaching into the drum, black pull, hinge left), the rubber boot and the perforated drum with three lifters
  behind the glass; the console: the detergent drawer (pull lip, compartment icons, blue drips under it), the red LED
  price "$3.25", four cycle buttons (HOT WARM COLD DELICATE), a coin drop with its return button and a retrofitted card
  reader with a lit LCD; the SPEED SPIN badge, machine number 07 (WF_Number), a CAUTION sticker, the rating plate; a
  dark kick plate scuffed by shoes and carts; leveling feet. Hand grime round the pull and the buttons.
- SM_WF_DryerStack (0.79 x 0.89 x 1.95): two stacked almond-enamel commercial dryers (machines 31 over 32, WF_Number).
  Domed glass doors in chrome bezels with bar handles, the felt seals, the drums (WF_DrumGlow) with their ribs, a
  forgotten load in the upper drum. The control band between them: per drum a red LED counter, a coin plate (25c = 6
  MIN), HIGH / MED / LOW and PUSH TO START; the lint door at the foot (louvers, a keyed lock, the yellow CLEAN LINT
  SCREEN sticker); the SPEED SPIN plate on the header. The enamel yellowed, wiped, scratched, scuffed low, chipped; lint and dust on top.
- SM_WF_FoldTable (2.40 x 0.90 x 0.93): a laminate folding table (flecked putty, a black vinyl T-molding, chips out of
  the laminate's edge, cup and detergent rings) on six grey steel legs with an apron and a lower shelf; on it a lost
  grey sock and two red $5 chips from the back room's game, a peeling NO SITTING ON TABLES sticker; on the shelf a
  LOST + FOUND carton (marker on masking tape) with a hoodie and a kid's shirt in it, a sleeve over the side.
- SM_WF_Chairs (2.17 x 0.53 x 0.85): four molded polypropylene shells (three sun-faded orange, one teal replacement)
  on brackets along a charcoal steel beam (each shell on four standoffs into molded bosses), on two T legs with
  glides; the seats and backs worn glossy where people sit.
- SM_WF_ChangeMachine (0.48 x 0.47 x 1.80): a bill changer on a pedestal. Lit CHANGE marquee (front and both sides),
  bill acceptor with green arrows, the bills it takes, a stainless coin cup with a few quarters left in it, a T-handle
  lock, instruction decals and a taped handwritten NO $50s note.
- SM_WF_SoapVend (0.72 x 0.51 x 1.72): a detergent vending machine. Five columns of single-load boxes (SUDZ, BRITE,
  FRESH, BLEACH, SOFTY) stacked behind the glass and lit by a tube, a price strip, five pull knobs, coin and bill
  acceptors, the coin return, the PUSH flap of the delivery bin, a lit LAUNDRY SUPPLIES header.
- SM_WF_Cart (0.77 x 0.53 x 1.75): a chrome wire laundry cart on four swivel casters with corner bumpers; the valet
  rail overhead with two wire hangers and a plastic one.
- SM_WF_Counter (2.04 x 0.67 x 1.05, things on it to 1.39): the attendant's counter (customer side -Y). A teal laminate
  front with a stainless bumper over a black vinyl base, a flecked top with rounded edges; on top a cash register
  (membrane keys toward the attendant, lit displays both ways, a receipt curling out), a receipt spike with slips, a
  platform scale (wash-and-fold goes by the pound), a call bell and its PLEASE RING FOR ATTENDANT tent card, two bagged
  orders (folded laundry stacked in WF_Plastic bags, knotted, yellow tickets taped on: Perez, Okafor); behind, the
  shelves: Dee's own order, a roll of bags, a spray bottle, a bulk jug, a carton. The WASH & FOLD price sign on the
  front.
- SM_WF_BackDoor (1.02 x 0.22 x 2.17): the steel door to the back room, closed in its hollow-metal frame. The frame
  wraps a 0.20 m wall centered on Y 0 (its faces at Y -0.108 / +0.108); the clear opening is 0.92 x 2.12 m (the back
  room's DoorX0..DoorX1 and DoorTop). The leaf (0.91 x 2.10 x 0.045, painted, chipped, scratched at cart height)
  swings toward +Y (into the back room) on three hinges on the left as seen from the laundromat. Front: kick plate, a
  touch-bar exit device, a deadbolt, the engraved PRIVATE / STAFF ONLY sign and Dee's index card "Tue Thu Sat 9 PM"
  taped on. Back: a pull and a closer. SM_WF_BackDoor_Frame and SM_WF_BackDoor_Leaf are the same two parts apart, to
  swing the leaf: the leaf's origin is its hinge axis on the floor; place it at the frame's origin + LEAF_PIVOT
  (Blender (-0.4575, 0.040, 0), i.e. Unreal (-45.75, -4.0, 0) cm in the frame's space) and yaw it about Z (positive
  Blender yaw opens it toward +Y).
- SM_WF_WallDressing (1.53 x 0.03, from 1.13 to 2.02 m up): a cork board in an aluminum frame with flyers (room for
  rent, guitar lessons with tear-off tabs, a lost cat, house cleaning, tax prep, a moving sale, found keys) and
  business cards on push pins; the OPEN 24 HOURS sign over it; ATTENDANT ON DUTY, NO DYEING and NOT RESPONSIBLE signs
  beside it.
- SM_WF_TV (0.74 x 0.20, from 1.60 to 2.34 m up): a 32" LCD on a tilting wall bracket, tipped 12 degrees down, its
  power and coax cables looping down to an outlet box on the wall.

WF_ONLY=washer,dryer,... (keys of BUILDERS) builds just those; WF_REVIEW=0 skips the close-up sheet
(art/review/wash_fold_detail.jpg: every mesh under fluorescent tubes on a tile floor) that pose_for_review renders
before the standard sheet (the kit stood up as a corner of the laundromat).
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

NAMES = {'washer': ['SM_WF_Washer'], 'dryer': ['SM_WF_DryerStack'], 'table': ['SM_WF_FoldTable'], 'chairs': ['SM_WF_Chairs'],
         'change': ['SM_WF_ChangeMachine'], 'vend': ['SM_WF_SoapVend'], 'cart': ['SM_WF_Cart'], 'counter': ['SM_WF_Counter'],
         'door': ['SM_WF_BackDoor', 'SM_WF_BackDoor_Frame', 'SM_WF_BackDoor_Leaf'], 'wall': ['SM_WF_WallDressing'], 'tv': ['SM_WF_TV']}
_ONLY = [s.strip() for s in os.environ.get('WF_ONLY', '').split(',') if s.strip()]
WHICH = [k for k in NAMES if not _ONLY or k in _ONLY]
MESHES = [n for k in WHICH for n in NAMES[k]]
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {
    'wfw_body': 2048, 'wfw_print': 1024,
    'wfd_body': 2048, 'wfd_print': 1024,
    'wft_body': 2048, 'wft_print': 512,
    'wfc_body': 2048,
    'wfx_body': 1024, 'wfx_print': 1024,
    'wfv_body': 2048, 'wfv_print': 1024,
    'wfk_body': 1024,
    'wfo_body': 2048, 'wfo_print': 2048,
    'wfbf_body': 1024, 'wfbl_body': 2048, 'wfbl_print': 1024,
    'wfg_body': 1024, 'wfg_print': 2048,
    'wftv_body': 1024, 'wftv_print': 256,
}
AO_DISTANCE = 0.05
REVIEW_SCREEN_LIGHT = False
REVIEW_VIEWS = None   # set by pose_for_review

UNBAKED = ('WF_Glass', 'WF_Plastic', 'WF_DrumGlow', 'WF_TVScreen', 'WF_Number')
NUM_CELLS = 32        # T_WF_Numbers: machine-number stickers 01..32, left to right
LEAF_PIVOT = (-0.4575, 0.0400, 0.0)   # SM_WF_BackDoor_Leaf's origin in the door's frame (its hinge axis on the floor)

# Body finishes, chosen per face by the wf_fin attribute (wf_col tints those that take a color).
FIN = {n: i + 1 for i, n in enumerate(['steel', 'chrome', 'enamel', 'plastic', 'gloss', 'rubber', 'kick', 'powder', 'laminate', 'cork',
                                       'alu', 'cloth', 'paper', 'bag', 'drum', 'paint', 'particle', 'felt', 'solid'])}


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


def set_attr_faces(obj, name, fn):
    """A per-face vector attribute from fn(face center) -> 3-tuple."""
    me = obj.data
    a = me.attributes.get(name) or me.attributes.new(name, 'FLOAT_VECTOR', 'FACE')
    vals = []
    for p in me.polygons:
        vals.extend(fn(p.center))
    a.data.foreach_set('vector', vals)


def bx(name, x0, x1, y0, y1, z0, z1, r=0.0, seg=2):
    return parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (x1 - x0, y1 - y0, z1 - z0), r, seg)


FRONT = Matrix.Rotation(math.radians(90.0), 4, 'X')   # a lathe's +Z (toward the viewer) -> Blender -Y


def at_front(x, y, z):
    return Matrix.Translation((x, y, z)) @ FRONT


def weld(obj, dist=1e-6):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=dist)
    bm.to_mesh(obj.data)
    bm.free()


def lathe_solid(name, profile, matrix, segments=64):
    """A closed solid of revolution from a closed (r, z) loop, moved by matrix."""
    obj = core.lathe(name, list(profile) + [profile[0]], segments)
    weld(obj)
    core.orient_normals(obj)
    obj.data.transform(matrix)
    return obj


def lathe_axis(name, profile, matrix, segments=48):
    """A solid of revolution whose (r, z) profile starts and ends on the axis."""
    obj = core.lathe(name, profile, segments)
    weld(obj)
    core.orient_normals(obj)
    obj.data.transform(matrix)
    return obj


def lathe_open(name, profile, matrix, segments=64, toward=(0.0, 0.0, 1.0)):
    """An open surface of revolution, its normals turned toward `toward` (lathe space) on average."""
    obj = core.lathe(name, profile, segments)
    core.orient_normals(obj, toward)
    obj.data.transform(matrix)
    return obj


def tube(name, pts, r, n=12, closed=False):
    return core.sweep(name, pts, parts.circle(r, n), closed=closed, caps=not closed)


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


def rrect_path(x0, y0, x1, y1, r, z, steps=6):
    return [(x, y, z) for x, y in core.rounded_rect(x0, y0, x1, y1, r, steps)]


def front_plate(name, outline, holes, y_front, t):
    """A plate in the XZ plane (outline and holes as (x, z) points), its front face at y_front, t thick toward +Y."""
    return parts.outline_prism(name, outline, holes, 0.0, t, Matrix.Translation((0.0, y_front + t, 0.0)) @ FRONT)


def circle_xz(cx, cz, r, n=64):
    return [(cx + r * math.cos(2 * math.pi * k / n), cz + r * math.sin(2 * math.pi * k / n)) for k in range(n)]


def disc_y(name, x, y, z, r, depth, n=24):
    """A short cylinder on the -Y axis: its back face at y, its front at y - depth."""
    return parts.rod(name, (x, y, z), (x, y - depth, z), r, n)


def frame_matrix(u, v, origin):
    """Local X -> u, local Y -> v, local Z -> u x v, at origin."""
    U, V = Vector(u).normalized(), Vector(v).normalized()
    N = U.cross(V).normalized()
    m = Matrix((U, V, N)).transposed().to_4x4()
    m.translation = Vector(origin)
    return m


def blob(name, center, size, seed=1, detail=0.18, subdiv=3, flatten=0.0, wrinkle=0.0, smooth=0):
    """A soft lump (laundry, a bag): an icosphere scaled to size and pushed about by noise; `smooth` passes of
    Laplacian smoothing soften the wrinkles' V creases into folds (cloth, not crumpled paper)."""
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


# ================================================================== the kit: parts, finishes, decals

class Kit:
    """One mesh's parts: body parts carry a finish (wf_fin) and a tint (wf_col); decals carry atlas UVs; unbaked
    parts carry one of the shared UNBAKED materials."""

    def __init__(self, prefix, name, dust=0.3, drum=None, low_grime=0.12):
        self.prefix, self.name = prefix, name
        self.objs = []
        self.kinds = set()
        self.atlas = None
        self.dust = dust
        self.drum = drum            # (x center, z center, radius) of a perforated drum along Y (finish 'drum')
        self.low = low_grime        # how high floor grime climbs (meters)
        self.grime = []             # (center, radius, amount): hand grime
        self.polish = []            # (center, radius, amount): worn glossy where people sit or hold (plastics)
        self.drips = []             # (x0, x1, z_top, length): detergent drips down a front
        self.laminate_chips = []    # (x, y, r): chips in a laminate top
        self.rings = []             # (x, y, r, hex): cup and detergent rings on a laminate top

    def add(self, obj, fin, col=0xffffff):
        set_attr(obj, 'wf_fin', float(FIN[fin]))
        set_attr(obj, 'wf_col', lin3(col))
        set_attr(obj, 'wf_lock', 0.0)
        obj['wf_slot'] = 'body'
        self.kinds.add(fin)
        self.objs.append(obj)
        return obj

    def special(self, obj, mat_name, locked=False):
        set_attr(obj, 'wf_fin', 0.0)
        set_attr(obj, 'wf_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'wf_lock', 1.0 if locked else 0.0)
        obj['wf_slot'] = mat_name
        self.objs.append(obj)
        return obj

    def panel(self, key, w, h, draw, glow=0.0, density=1.0):
        """A print panel w x h meters (drawn in millimeters)."""
        if self.atlas is None:
            self.atlas = Atlas(f'wf_{self.prefix}_print', TEXTURE_SIZES.get(f'{self.prefix}_print', 1024))
        self.atlas.add(key, w * 1000.0, h * 1000.0, draw, glow, density)

    def decal(self, key, origin, u=(1, 0, 0), v=(0, 0, 1), w=None, h=None, lift=0.0004, outline=None, nu=1, nv=1, disp=None, matrix=None):
        """Panel `key` laid on the plane through origin spanned by u (the print's right) and v (its up), facing u x v.
        outline: (s, t) points in [0, 1]^2 of the panel for a shaped decal; nu x nv: a grid, displaced along the
        normal by disp(s, t) (curled paper)."""
        p = self.atlas.panels[key]
        w = p['w'] / 1000.0 if w is None else w
        h = p['h'] / 1000.0 if h is None else h
        u0, v0, u1, v1 = self.atlas.rect(key)
        U, V = Vector(u).normalized(), Vector(v).normalized()
        N = U.cross(V).normalized()
        O = Vector(origin) + N * lift
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new('UVMap')

        def pos(s, t):
            d = disp(s, t) if disp else 0.0
            return O + U * ((s - 0.5) * w) + V * ((t - 0.5) * h) + N * d

        if outline:
            vs = [bm.verts.new(pos(s, t)) for s, t in outline]
            faces = [(bm.faces.new(vs), outline)]
            for f, st in faces:
                for loop, (s, t) in zip(f.loops, st):
                    loop[uvl].uv = (u0 + (u1 - u0) * s, v0 + (v1 - v0) * t)
        else:
            grid = [[bm.verts.new(pos(i / nu, j / nv)) for i in range(nu + 1)] for j in range(nv + 1)]
            for j in range(nv):
                for i in range(nu):
                    f = bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
                    for loop, (a, b) in zip(f.loops, ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))):
                        loop[uvl].uv = (u0 + (u1 - u0) * a / nu, v0 + (v1 - v0) * b / nv)
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        for f in bm.faces:
            if f.normal.dot(N) < 0:
                f.normal_flip()
        obj = core.mesh_object(f'decal_{key}', bm)
        if matrix is not None:
            obj.data.transform(matrix)
        set_attr(obj, 'wf_fin', 0.0)
        set_attr(obj, 'wf_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'wf_lock', 1.0)
        obj['wf_slot'] = 'print'
        self.objs.append(obj)
        return obj

    def locked_print(self, obj, key, uvfun):
        """Any mesh printed with panel `key`: uvfun(co) -> (s, t) in [0, 1]^2 of the panel."""
        u0, v0, u1, v1 = self.atlas.rect(key)
        me = obj.data
        uv = me.uv_layers.get('UVMap') or me.uv_layers.new(name='UVMap')
        for poly in me.polygons:
            for li in poly.loop_indices:
                s, t = uvfun(me.vertices[me.loops[li].vertex_index].co)
                uv.data[li].uv = (u0 + (u1 - u0) * s, v0 + (v1 - v0) * t)
        set_attr(obj, 'wf_fin', 0.0)
        set_attr(obj, 'wf_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'wf_lock', 1.0)
        obj['wf_slot'] = 'print'
        self.objs.append(obj)
        return obj

    def finish(self, angle=35.0, name=None):
        name = name or self.name
        mats = [body_material(self)]
        if any(o['wf_slot'] == 'print' for o in self.objs):
            mats.append(print_material(self))
        for n in UNBAKED:
            if any(o['wf_slot'] == n for o in self.objs):
                mats.append(UNB[n])
        by = {'body': mats[0]}
        if len(mats) > 1 and mats[1].m.name.endswith('_print'):
            by['print'] = mats[1]
        for o in self.objs:
            slot = o['wf_slot']
            core.assign(o, by[slot] if slot in by else UNB[slot])
            if not o.data.uv_layers:
                o.data.uv_layers.new(name='UVMap')
            elif o.data.uv_layers[0].name != 'UVMap':
                o.data.uv_layers[0].name = 'UVMap'
        obj = core.join(self.objs, name) if len(self.objs) > 1 else self.objs[0]
        obj.name = name
        obj.data.name = name
        order_slots(obj, [m.m if isinstance(m, core.Mat) else m for m in mats])
        core.finish_hard_surface(obj, angle)
        lock = obj.data.attributes.get('wf_lock')
        vals = np.zeros(len(obj.data.polygons), dtype=np.float32)
        lock.data.foreach_get('value', vals)
        locked = set(np.nonzero(vals > 0.5)[0].tolist())
        groups = [(lambda f: f.index in locked, 'keep', None, (0.0, 0.0, 1.0, 1.0))]
        for s in obj.material_slots:
            groups.append((core.material_is(obj, s.material.name), 'box', None, (0.0, 0.0, 1.0, 1.0)))
        groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
        core.uv_layout(obj, groups)
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        print(f'[wash_fold] {name}: {tris} triangles, slots {[s.material.name for s in obj.material_slots]}')
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

    def ring(self, cx, cy, r0, r1, color, metal=0.0, rough=0.4, n=64, a0=0.0, a1=2 * math.pi):
        X, Y = self.P(cx, cy)
        return self.s.ring(X, Y, r0 * self.k, r1 * self.k, color, metal, rough, n, a0, a1)

    def text(self, txt, x, y, size, color, face='Black', align='LEFT', rot=0.0, tracking=1.0, metal=0.0, rough=0.4):
        X, Y = self.P(x, y)
        return self.s.text(txt, X, Y, size * self.k, color, face=face, align=align, rot=rot, tracking=tracking, metal=metal, rough=rough)

    def fit(self, txt, x, y, size, max_w, color, face='Black', align='CENTER', tracking=1.0, metal=0.0, rough=0.4, rot=0.0):
        size = min(size, max_w / text_w(self.s, txt, face, tracking))
        self.text(txt, x, y, size, color, face, align, rot, tracking, metal, rough)
        return size

    def mid(self, txt, x, cy, size, max_w, color, face='Black', align='CENTER', tracking=1.0, metal=0.0, rough=0.4):
        size = min(size, max_w / text_w(self.s, txt, face, tracking))
        self.text(txt, x, cy - 0.355 * size, size, color, face, align, 0.0, tracking, metal, rough)
        return size

    def mesh(self, verts, faces, color, metal=0.0, rough=0.5):
        me = bpy.data.meshes.new('poly')
        me.from_pydata([(self.P(x, y)[0], self.P(x, y)[1], 0.0) for x, y in verts], [], faces)
        return self.s._add(bpy.data.objects.new('poly', me), color, metal, rough)

    def stroke(self, pts, width, color, rough=0.6):
        """A pen line through pts (rounded joints)."""
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
            self.mesh(verts, faces, color, 0.0, rough)


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

    def __init__(self, name, px, gap=0.008):
        self.name, self.px, self.gap = name, px, gap
        self.panels = {}
        self.rects = {}
        self.color = self.surface = None

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
        lo, hi = 0.0, 1.0
        while self._pack(hi) is not None:
            hi *= 2.0
        for _ in range(40):
            mid = (lo + hi) / 2
            if self._pack(mid) is not None:
                lo = mid
            else:
                hi = mid
        pos = self._pack(lo)
        s = Sheet(1000.0, 1000.0)
        for k, (x, y, w, h) in pos.items():
            p = self.panels[k]
            p['draw'](Sub(s, x * 1000.0, y * 1000.0, w * 1000.0 / p['w']), p['w'], p['h'])
            self.rects[k] = (x, y, x + w, y + h)
        self.color, self.surface = s.render(self.name, self.px, samples=24)
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
        return self

    def rect(self, key):
        """The panel's UV rect, inset half a texel."""
        x0, y0, x1, y1 = self.rects[key]
        e = 0.6 / self.px
        return (x0 + e, y0 + e, x1 - e, y1 - e)


# ------------------------------------------------------------------ handwriting (a pen font of strokes)

_HAND = {
    'T': ([[(0.0, 0.70), (0.26, 0.72), (0.54, 0.73)], [(0.27, 0.72), (0.26, 0.36), (0.25, 0.0)]], 0.52),
    'u': ([[(0.04, 0.50), (0.04, 0.16), (0.10, 0.03), (0.22, 0.0), (0.34, 0.06), (0.41, 0.22), (0.42, 0.5), (0.42, 0.22), (0.43, 0.04), (0.48, 0.0)]], 0.52),
    'e': ([[(0.06, 0.25), (0.36, 0.28), (0.40, 0.38), (0.32, 0.48), (0.18, 0.50), (0.07, 0.40), (0.03, 0.22), (0.10, 0.06), (0.24, 0.0), (0.40, 0.06)]], 0.46),
    'h': ([[(0.05, 0.78), (0.05, 0.36), (0.05, 0.0)], [(0.05, 0.28), (0.14, 0.44), (0.28, 0.50), (0.38, 0.42), (0.40, 0.26), (0.40, 0.0)]], 0.50),
    'a': ([[(0.38, 0.40), (0.28, 0.49), (0.14, 0.48), (0.04, 0.34), (0.04, 0.14), (0.12, 0.02), (0.26, 0.02), (0.38, 0.16)],
           [(0.39, 0.50), (0.39, 0.10), (0.46, 0.0)]], 0.52),
    't': ([[(0.16, 0.70), (0.16, 0.10), (0.22, 0.0), (0.32, 0.03)], [(0.02, 0.47), (0.18, 0.48), (0.34, 0.50)]], 0.38),
    's': ([[(0.36, 0.44), (0.24, 0.50), (0.10, 0.47), (0.06, 0.37), (0.15, 0.28), (0.30, 0.22), (0.38, 0.11), (0.29, 0.01), (0.13, 0.0), (0.03, 0.07)]], 0.44),
    'S': ([[(0.46, 0.64), (0.36, 0.72), (0.18, 0.72), (0.06, 0.62), (0.08, 0.47), (0.26, 0.38), (0.42, 0.28), (0.47, 0.14), (0.37, 0.02), (0.18, 0.0), (0.04, 0.08)]], 0.54),
    'P': ([[(0.06, 0.0), (0.06, 0.36), (0.06, 0.72), (0.30, 0.72), (0.44, 0.64), (0.46, 0.50), (0.38, 0.40), (0.24, 0.37), (0.06, 0.37)]], 0.50),
    'M': ([[(0.04, 0.0), (0.05, 0.36), (0.06, 0.72), (0.30, 0.24), (0.54, 0.72), (0.55, 0.36), (0.56, 0.0)]], 0.64),
    '9': ([[(0.40, 0.50), (0.31, 0.38), (0.18, 0.36), (0.06, 0.46), (0.06, 0.62), (0.18, 0.72), (0.34, 0.72), (0.42, 0.62), (0.42, 0.40), (0.36, 0.12),
            (0.24, 0.0), (0.10, 0.04)]], 0.50),
    'D': ([[(0.06, 0.0), (0.06, 0.36), (0.06, 0.72)], [(0.06, 0.72), (0.26, 0.71), (0.42, 0.60), (0.48, 0.38), (0.42, 0.14), (0.26, 0.02), (0.06, 0.0)]], 0.54),
    'N': ([[(0.05, 0.0), (0.05, 0.36), (0.05, 0.72), (0.25, 0.36), (0.45, 0.0), (0.45, 0.36), (0.45, 0.72)]], 0.52),
    'O': ([[(0.28, 0.72), (0.12, 0.66), (0.04, 0.48), (0.04, 0.24), (0.12, 0.06), (0.28, 0.0), (0.44, 0.06), (0.52, 0.24), (0.52, 0.48), (0.44, 0.66),
            (0.28, 0.72), (0.22, 0.70)]], 0.58),
    '0': ([[(0.24, 0.72), (0.10, 0.64), (0.04, 0.40), (0.06, 0.14), (0.16, 0.0), (0.32, 0.0), (0.42, 0.14), (0.44, 0.40), (0.38, 0.64), (0.24, 0.72),
            (0.18, 0.69)]], 0.50),
    '5': ([[(0.42, 0.72), (0.12, 0.72), (0.08, 0.42), (0.22, 0.46), (0.36, 0.42), (0.44, 0.27), (0.40, 0.09), (0.26, 0.0), (0.09, 0.05)]], 0.50),
    '$': ([[(0.40, 0.58), (0.30, 0.64), (0.16, 0.64), (0.06, 0.55), (0.10, 0.43), (0.26, 0.36), (0.38, 0.27), (0.40, 0.14), (0.30, 0.06), (0.15, 0.06),
            (0.04, 0.13)], [(0.23, 0.76), (0.23, 0.36), (0.22, -0.06)]], 0.48),
    'r': ([[(0.05, 0.50), (0.05, 0.25), (0.05, 0.0)], [(0.05, 0.30), (0.12, 0.44), (0.24, 0.50), (0.34, 0.47)]], 0.36),
    'z': ([[(0.04, 0.49), (0.20, 0.50), (0.36, 0.50), (0.20, 0.25), (0.04, 0.01), (0.22, 0.0), (0.38, 0.0)]], 0.42),
    'k': ([[(0.05, 0.76), (0.05, 0.36), (0.05, 0.0)], [(0.36, 0.50), (0.06, 0.22), (0.38, 0.0)]], 0.42),
    'f': ([[(0.36, 0.72), (0.26, 0.76), (0.16, 0.70), (0.13, 0.55), (0.13, 0.25), (0.12, 0.0)], [(0.0, 0.46), (0.30, 0.47)]], 0.34),
    'o': ([[(0.22, 0.50), (0.08, 0.43), (0.03, 0.25), (0.08, 0.07), (0.22, 0.0), (0.36, 0.07), (0.41, 0.25), (0.36, 0.43), (0.22, 0.50), (0.17, 0.48)]], 0.46),
    'n': ([[(0.05, 0.50), (0.05, 0.25), (0.05, 0.0)], [(0.05, 0.30), (0.14, 0.45), (0.28, 0.50), (0.38, 0.42), (0.40, 0.26), (0.40, 0.0)]], 0.48),
    'i': ([[(0.08, 0.48), (0.08, 0.25), (0.08, 0.02), (0.12, 0.0)], [(0.08, 0.66), (0.085, 0.67)]], 0.20),
    'l': ([[(0.08, 0.76), (0.08, 0.40), (0.08, 0.05), (0.13, 0.0)]], 0.20),
    'c': ([[(0.38, 0.42), (0.26, 0.50), (0.12, 0.46), (0.04, 0.30), (0.06, 0.12), (0.18, 0.0), (0.32, 0.02), (0.40, 0.10)]], 0.44),
    'd': ([[(0.38, 0.40), (0.28, 0.49), (0.14, 0.48), (0.04, 0.34), (0.04, 0.14), (0.12, 0.02), (0.26, 0.02), (0.38, 0.16)],
           [(0.40, 0.78), (0.40, 0.36), (0.40, 0.08), (0.46, 0.0)]], 0.50),
    'm': ([[(0.04, 0.50), (0.04, 0.25), (0.04, 0.0)], [(0.04, 0.30), (0.12, 0.46), (0.22, 0.50), (0.30, 0.42), (0.31, 0.0)],
           [(0.31, 0.32), (0.40, 0.47), (0.50, 0.50), (0.58, 0.42), (0.59, 0.0)]], 0.66),
    'L': ([[(0.06, 0.72), (0.06, 0.36), (0.06, 0.0), (0.26, 0.0), (0.44, 0.01)]], 0.46),
    'F': ([[(0.44, 0.73), (0.26, 0.72), (0.06, 0.72), (0.06, 0.36), (0.06, 0.0)], [(0.06, 0.38), (0.34, 0.39)]], 0.46),
    'U': ([[(0.06, 0.72), (0.06, 0.24), (0.14, 0.04), (0.26, 0.0), (0.38, 0.04), (0.46, 0.24), (0.46, 0.72)]], 0.52),
    '+': ([[(0.06, 0.34), (0.40, 0.35)], [(0.23, 0.52), (0.23, 0.17)]], 0.46),
    'y': ([[(0.04, 0.50), (0.10, 0.24), (0.21, 0.02)], [(0.40, 0.50), (0.28, 0.16), (0.16, -0.14), (0.08, -0.24), (0.0, -0.25)]], 0.44),
    'g': ([[(0.38, 0.40), (0.28, 0.49), (0.14, 0.48), (0.04, 0.34), (0.04, 0.16), (0.12, 0.04), (0.26, 0.04), (0.38, 0.18)],
           [(0.39, 0.50), (0.39, 0.10), (0.36, -0.14), (0.24, -0.24), (0.08, -0.20)]], 0.50),
    'b': ([[(0.05, 0.78), (0.05, 0.36), (0.05, 0.0)], [(0.05, 0.26), (0.14, 0.44), (0.28, 0.50), (0.39, 0.40), (0.42, 0.22), (0.35, 0.05),
                                                     (0.21, 0.0), (0.05, 0.05)]], 0.48),
    ':': ([[(0.08, 0.40), (0.09, 0.41)], [(0.07, 0.03), (0.08, 0.04)]], 0.20),
    '.': ([[(0.08, 0.02), (0.09, 0.03)]], 0.22),
    '-': ([[(0.04, 0.30), (0.20, 0.31), (0.36, 0.30)]], 0.42),
}


def hand(s, text, x, y, size, color, width=0.075, slant=0.14, seed=1, smooth=4):
    """Handwritten text (ballpoint or marker strokes): baseline at y, size = the pen's cap-to-baseline scale."""
    rnd = random.Random(seed)
    cx = x
    for ch in text:
        if ch == ' ':
            cx += 0.30 * size
            continue
        strokes, adv = _HAND[ch]
        js = rnd.uniform(0.94, 1.06)
        jx, jy = rnd.uniform(-0.03, 0.03), rnd.uniform(-0.04, 0.03)
        for st in strokes:
            pts = [(cx + (px * js + py * js * slant + jx + rnd.uniform(-0.012, 0.012)) * size, y + (py * js + jy + rnd.uniform(-0.012, 0.012)) * size)
                   for px, py in st]
            if len(pts) > 2:
                pts = [tuple(p) for p in core.catmull_rom(pts, smooth)]
            s.stroke(pts, width * size * rnd.uniform(0.9, 1.1), color)
        cx += (adv * js + 0.05) * size
    return cx


def hand_w(text, size):
    return sum((0.30 if ch == ' ' else _HAND[ch][1] + 0.05) for ch in text) * size


# ------------------------------------------------------------------ shared unbaked materials

UNB = {}


def make_unbaked():
    """The slots the game drives, one material each, shared by every mesh (so the names never pick up a .001)."""
    g = core.Mat('WF_Glass')
    g.set('Base Color', H(0x26302f))
    g.set('Roughness', 0.03)
    g.set('Metallic', 0.0)
    g.set('Alpha', 0.2)
    g.set('Specular IOR Level', 0.6)
    g.m.use_backface_culling = True
    try:
        g.m.surface_render_method = 'BLENDED'
    except (AttributeError, TypeError):
        pass
    UNB['WF_Glass'] = g.m
    pl = core.Mat('WF_Plastic')
    pl.set('Base Color', H(0xe9eef1))
    pl.set('Roughness', 0.22)
    pl.set('Metallic', 0.0)
    pl.set('Alpha', 0.42)
    pl.set('Specular IOR Level', 0.5)
    pl.m.use_backface_culling = True
    try:
        pl.m.surface_render_method = 'BLENDED'
    except (AttributeError, TypeError):
        pass
    UNB['WF_Plastic'] = pl.m
    d = core.Mat('WF_DrumGlow')
    glow_path, base_path = drum_texture()
    img = d.image(glow_path)
    base = d.image(base_path)
    d.link(base.outputs['Color'], d.bsdf.inputs['Base Color'])
    d.link(img.outputs['Color'], d.bsdf.inputs['Emission Color'])
    d.set('Emission Strength', DRUM_GLOW)
    d.set('Roughness', 0.45)
    d.set('Metallic', 0.0)
    d.m.use_backface_culling = True
    UNB['WF_DrumGlow'] = d.m
    t = core.Mat('WF_TVScreen')
    timg = t.image(tv_picture())
    t.set('Base Color', H(0x040506))
    t.link(timg.outputs['Color'], t.bsdf.inputs['Emission Color'])
    t.set('Emission Strength', 2.5)
    t.set('Roughness', 0.18)
    t.m.use_backface_culling = True
    UNB['WF_TVScreen'] = t.m
    nm = core.Mat('WF_Number')
    nimg = nm.image(number_strip())
    nm.link(nimg.outputs['Color'], nm.bsdf.inputs['Base Color'])
    nm.set('Roughness', 0.42)
    nm.set('Metallic', 0.0)
    nm.m.use_backface_culling = True
    UNB['WF_Number'] = nm.m
    for n, mt in UNB.items():
        if mt.name != n:
            raise RuntimeError(f'material name clash: wanted {n}, got {mt.name}')


# Dryer drum (dryer_drum below): the lathe profile its glow texture is drawn for.
DRUM_R, DRUM_MOUTH, DRUM_DEPTH = 0.36, 0.25, 0.62
DRUM_PROFILE = [(DRUM_MOUTH, -0.045), (DRUM_R, -0.06), (DRUM_R, -0.06 - DRUM_DEPTH + 0.015), (0.0, -0.06 - DRUM_DEPTH + 0.015)]
DRUM_V = 0.88   # the lathe's v is squeezed into 0..DRUM_V; the ribs take the strip above
DRUM_GLOW = 0.3   # idle: the drums read dark behind the glass (the brief); a running drum's lamp is ~1.5-2


def drum_texture():
    """The dryer drum's inside as seen lit: perforated stainless (holes dark), the back plate's rings of holes round a
    hub, the ribs' strip. Rows bottom-up, u round the drum, v along DRUM_PROFILE by arc length."""
    W, Hh = 2048, 1024
    seg = [math.hypot(b[0] - a[0], b[1] - a[1]) for a, b in zip(DRUM_PROFILE, DRUM_PROFILE[1:])]
    total = sum(seg)
    vb = [0.0, seg[0] / total, (seg[0] + seg[1]) / total, 1.0]
    u = (np.arange(W) + 0.5) / W
    v = (np.arange(Hh) + 0.5) / Hh
    U, V = np.meshgrid(u, v)
    vv = V / DRUM_V
    base = np.full((Hh, W), 0.62)
    # brushed steel: streaks round the drum
    P = np.stack([U * 40.0, vv * 3.0, np.zeros_like(U)], axis=-1).reshape(-1, 3)
    streak = texmaps.value_noise(P, 0.05, seed=3, octaves=2).reshape(Hh, W)
    base += (streak - 0.5) * 0.12
    holes = np.zeros((Hh, W))
    cyl = (vv >= vb[1]) & (vv < vb[2])
    # cylinder: staggered holes, 18 mm pitch, 3.2 mm radius
    s_m = U * 2 * math.pi * DRUM_R
    d_m = (vv - vb[1]) / (vb[2] - vb[1]) * seg[1]
    pitch = 0.018
    row = np.floor(d_m / pitch)
    off = np.where(row % 2 == 0, 0.0, pitch / 2)
    ds = (s_m + off) % pitch - pitch / 2
    dd = d_m % pitch - pitch / 2
    dist = np.hypot(ds, dd)
    px = 2 * math.pi * DRUM_R / W
    holes = np.where(cyl, texmaps.smoothstep(0.0032 + px * 0.6, 0.0032 - px * 0.6, dist), holes)
    # back plate: rings of holes, a hub
    back = (vv >= vb[2]) & (vv <= 1.0)
    rr = (1.0 - (vv - vb[2]) / (1.0 - vb[2])) * DRUM_R
    th = U * 2 * math.pi
    ring_i = np.round(rr / 0.022)
    Ri = ring_i * 0.022
    n_i = np.maximum(np.floor(2 * math.pi * Ri / 0.022), 1)
    th_h = np.round(th / (2 * math.pi) * n_i) / n_i * 2 * math.pi
    dist_b = np.hypot(rr - Ri, Ri * (th - th_h))
    hb = texmaps.smoothstep(0.0035, 0.0022, dist_b) * (ring_i >= 3) * (rr < DRUM_R - 0.03)
    holes = np.where(back, hb, holes)
    hub = back & (rr < 0.055)
    base = np.where(hub, 0.5 + (streak - 0.5) * 0.1, base)
    rim = back & (rr > DRUM_R - 0.02)
    base = np.where(rim, base * 0.8, base)
    # mouth ring: plain, a little darker toward the door
    mouth = vv < vb[1]
    base = np.where(mouth, base * (0.55 + 0.35 * vv / max(vb[1], 1e-3)), base)
    # light falls off toward the mouth: brighter at the back
    shade = np.where(cyl, 0.62 + 0.38 * (d_m / seg[1]), 1.0)
    shade = np.where(back, 1.0 - 0.15 * rr / DRUM_R, shade)
    lum = base * shade * (1.0 - holes * 0.92)
    # the ribs' strip
    ribs = V > DRUM_V + 0.01
    lum = np.where(ribs, 0.42 + (streak - 0.5) * 0.12, lum)
    lum = np.clip(lum, 0.0, 1.0)
    tint = np.array(lin3(0xfff0dc))
    rgb = lum[..., None] * tint[None, None, :]
    return texmaps.save('T_WF_DrumGlow', rgb, 'sRGB'), texmaps.save('T_WF_DrumGlow_Base', rgb * 0.42, 'sRGB')


def number_strip():
    """The machine-number stickers, 01..NUM_CELLS in a row of square cells (128 px each): an off-white vinyl disc, a
    thin black ring, the number; a little scuffed and yellowed, each its own way."""
    cell = 128.0
    s = Sheet(cell * NUM_CELLS, cell)
    rnd = random.Random(32)
    for n in range(1, NUM_CELLS + 1):
        cx, cy, r = (n - 0.5) * cell, cell / 2, cell / 2
        s.circle(cx, cy, r, 0xc9c4b6, rough=0.45, n=64)
        s.circle(cx, cy, r - 1.5, [0xece9df, 0xe8e3d4, 0xefece3, 0xe4dfcf][n % 4], rough=0.42, n=64)
        s.ring(cx, cy, r - 7.2, r - 5.0, 0x161616, rough=0.4, n=96)
        label = f'{n:02d}'
        size = min(64.0, 0.66 * cell / text_w(s, label, 'Black'))
        s.text(label, cx, cy - 0.355 * size, size, 0x141414, 'Black', align='CENTER')
        for _ in range(rnd.randint(2, 5)):
            a, d = rnd.uniform(0, 2 * math.pi), rnd.uniform(0.0, r * 0.75)
            s.rect(cx + d * math.cos(a), cy + d * math.sin(a), rnd.uniform(6, 22), rnd.uniform(0.6, 1.6), 0x8a8578, rough=0.6,
                   rot=rnd.uniform(-0.5, 0.5), alpha=rnd.uniform(0.15, 0.35))
        s.ring(cx, cy, r - 3.0, r - 1.5, 0xa39c88, rough=0.5, n=64, alpha=0.5)
    color, _surface = s.render('wf_numbers', int(cell * NUM_CELLS), samples=16)
    for obj, *_ in s.items:
        bpy.data.objects.remove(obj)
    path = os.path.join(os.path.dirname(color), 'T_WF_Numbers.png')   # the glTF names the image after its file
    shutil.copyfile(color, path)
    return path


def num_decal(k, n, origin, diameter, u=(1, 0, 0), v=(0, 0, 1), lift=0.0004, segs=40):
    """Machine-number sticker n (WF_Number): a disc on the plane through origin spanned by u, v, UV-mapped to cell n."""
    U, V = Vector(u).normalized(), Vector(v).normalized()
    N = U.cross(V).normalized()
    O = Vector(origin) + N * lift
    e = 0.6 / (128.0 * NUM_CELLS)
    u0, u1 = (n - 1) / NUM_CELLS + e, n / NUM_CELLS - e
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new('UVMap')
    ring = [(math.cos(2 * math.pi * i / segs), math.sin(2 * math.pi * i / segs)) for i in range(segs)]
    f = bm.faces.new([bm.verts.new(O + U * (cs * diameter / 2) + V * (sn * diameter / 2)) for cs, sn in ring])
    for loop, (cs, sn) in zip(f.loops, ring):
        loop[uvl].uv = (u0 + (u1 - u0) * (0.5 + 0.5 * cs), 0.004 + 0.992 * (0.5 + 0.5 * sn))
    if f.normal.dot(N) < 0:
        f.normal_flip()
    obj = core.mesh_object(f'number_{n:02d}', bm)
    return k.special(obj, 'WF_Number', locked=True)


def tv_picture():
    """A late-night poker broadcast on the laundromat's TV: the table from above, the board, a lower third."""
    w, h = 1280.0, 720.0
    s = Sheet(w, h)
    s.rect(0, 0, w, h, 0x0b1a16)
    for k in range(10):
        s.rect(0, h - (k + 1) * 72, w, 72, [0x0d1f1b, 0x0b1a16][k % 2])
    # the table: rail and felt in perspective
    pts = core.circle_points(640, 400, 1, 96)
    s.poly([(640 + (x - 640) * 560, 400 + (y - 400) * 250) for x, y in pts], 0x1c0f0a)
    s.poly([(640 + (x - 640) * 520, 400 + (y - 400) * 218) for x, y in pts], 0x1d6b42)
    s.poly([(640 + (x - 640) * 470, 400 + (y - 400) * 185) for x, y in pts], 0x217a4b)
    # the board
    ranks = [('A', 0xc0161c, 'h'), ('K', 0x101010, 's'), ('7', 0xc0161c, 'd'), ('7', 0x101010, 'c'), ('2', 0xc0161c, 'h')]
    for i, (r, col, suit) in enumerate(ranks):
        x0 = 640 - 2.5 * 92 + i * 92 + 6
        s.rect(x0, 350, 80, 112, 0xf4f2ec)
        s.text(r, x0 + 10, 425, 30, col, 'Bold')
        suit_shape(s, x0 + 46, 382, 22, suit, col)
    # chips in the pot
    for i in range(7):
        s.circle(560 + (i % 4) * 18, 300 - (i // 4) * 10, 14, [0xb51f1f, 0x1f3fb5, 0x111111, 0x2e8b3a][i % 4])
        s.ring(560 + (i % 4) * 18, 300 - (i // 4) * 10, 9, 12, 0xeeeeee)
    # lower third
    s.rect(0, 0, w, 150, 0x0a0d18)
    s.rect(0, 150, w, 6, 0xe0b44a)
    s.rect(40, 92, 360, 44, 0xe0b44a)
    s.text("NIGHT OWL HOLD'EM", 56, 103, 26, 0x0a0d18, 'Black')
    for j, (name, stack, cards) in enumerate((('D. MORALES', '1,240,000', ('A', 'Q')), ('K. OYELARAN', '860,000', ('9', '9')))):
        y0 = 54 - j * 42
        s.text(name, 60, y0, 24, 0xf2efe6, 'Bold')
        s.text(stack, 520, y0, 24, 0xe0b44a, 'Bold', align='RIGHT')
        for c, rk in enumerate(cards):
            s.rect(540 + c * 34, y0 - 6, 30, 36, 0xf4f2ec)
            s.text(rk, 548 + c * 34, y0 + 2, 20, 0xc0161c if c == 0 else 0x101010, 'Bold')
    s.text('POT  2,150,000', 1240, 100, 28, 0xf2efe6, 'Black', align='RIGHT')
    s.text('BLINDS 25K / 50K', 1240, 60, 20, 0x9aa3b8, 'Bold', align='RIGHT')
    s.text('ON THE RIVER', 1240, 26, 20, 0x9aa3b8, 'Bold', align='RIGHT')
    s.rect(1120, 640, 120, 44, 0xc0161c)
    s.text('LIVE', 1180, 652, 24, 0xffffff, 'Black', align='CENTER')
    color, _surface = s.render('wf_tv_picture', 1280, samples=16)
    for obj, *_ in s.items:
        bpy.data.objects.remove(obj)
    return color


def suit_shape(s, cx, cy, r, suit, col):
    if suit == 'd':
        s.poly([(cx, cy + r), (cx + r * 0.7, cy), (cx, cy - r), (cx - r * 0.7, cy)], col)
    elif suit == 'h':
        s.circle(cx - r * 0.38, cy + r * 0.28, r * 0.42, col)
        s.circle(cx + r * 0.38, cy + r * 0.28, r * 0.42, col)
        s.poly([(cx - r * 0.78, cy + r * 0.18), (cx + r * 0.78, cy + r * 0.18), (cx, cy - r * 0.8)], col)
    elif suit == 's':
        s.circle(cx - r * 0.38, cy - r * 0.05, r * 0.4, col)
        s.circle(cx + r * 0.38, cy - r * 0.05, r * 0.4, col)
        s.poly([(cx - r * 0.76, cy + r * 0.02), (cx + r * 0.76, cy + r * 0.02), (cx, cy + r * 0.85)], col)
        s.poly([(cx - r * 0.25, cy - r * 0.8), (cx + r * 0.25, cy - r * 0.8), (cx, cy - r * 0.1)], col)
    else:
        for dx, dy in ((0, 0.38), (-0.4, -0.05), (0.4, -0.05)):
            s.circle(cx + dx * r, cy + dy * r, r * 0.36, col)
        s.poly([(cx - r * 0.22, cy - r * 0.8), (cx + r * 0.22, cy - r * 0.8), (cx, cy)], col)


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
        self.fin = attr(m, 'wf_fin')
        self.col = attr(m, 'wf_col', 'Color')


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


def is_k(m, s, k):
    return m.math('LESS_THAN', m.math('ABSOLUTE', m.math('SUBTRACT', s, float(k))), 0.5)


def tint(m, col, amount, hexc):
    """col moved toward hexc by amount (0..1, socket or float)."""
    return m.mix(amount, col, H(hexc))


def lighten(m, col, amount):
    return m.mix(amount, col, (1.0, 1.0, 1.0, 1.0))


def darken(m, col, f):
    return m.mix(1.0, col, (f, f, f, 1.0), blend='MULTIPLY')


def f_steel(c):
    m = c.m
    br = noise(m, scaled(m, c.sep, 4.0, 400.0, 900.0), detail=3.0)
    fine = noise(m, scaled(m, c.sep, 20.0, 2400.0, 3000.0), detail=1.0)
    # Wiped stainless: faint smears and fingerprints, not clouds (big soft patches read as camouflage on a lid).
    smudge = ramp(m, noise(m, c.P, scale=11.0, detail=4.0), 0.56, 0.8)
    prints = ramp(m, noise(m, c.P, scale=40.0, detail=3.0, distortion=1.2), 0.62, 0.72)
    wipe = ramp(m, noise(m, scaled(m, c.sep, 3.0, 60.0, 60.0), detail=3.0), 0.55, 0.75)
    col = m.mix(br, H(0x8a8e92), H(0xb4b8bb))
    col = m.mix(mul(m, smudge, 0.14), col, H(0x74777a))
    rough = add(m, 0.22, mul(m, br, 0.05), mul(m, fine, 0.06), mul(m, smudge, 0.07), mul(m, prints, 0.08), mul(m, wipe, 0.05))
    return col, rough, 1.0, mul(m, fine, 0.004)


def f_chrome(c):
    m = c.m
    sm = ramp(m, noise(m, c.P, scale=12.0, detail=4.0), 0.45, 0.8)
    pit = ramp(m, noise(m, c.P, scale=600.0, detail=2.0), 0.72, 0.8)
    col = m.mix(mul(m, pit, 0.6), H(0xd0d2d4), H(0x77736c))
    rough = add(m, 0.05, mul(m, sm, 0.12), mul(m, pit, 0.25))
    return col, rough, 1.0, mul(m, pit, -0.01)


def isolines(m, vector, scale, width, detail=1.0):
    """Thin wandering lines (scratches, creases): where a noise crosses its middle."""
    n = noise(m, vector, scale=scale, detail=detail)
    return ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', n, 0.5)), width, 0.0)


def f_enamel(c):
    """Baked enamel on a machine's cabinet: orange peel, yellowed unevenly, wiped in arcs, scratched, scuffed low by
    carts and shoes, chipped on the corners."""
    m = c.m
    peel = noise(m, c.P, scale=500.0, detail=1.0)
    age = ramp(m, noise(m, c.P, scale=2.5, detail=3.0), 0.3, 0.75)
    col = m.mix(mul(m, age, 0.6), c.col, darken(m, c.col, 0.84))
    wipe = ramp(m, noise(m, scaled(m, c.sep, 50.0, 50.0, 3.0), detail=3.0), 0.56, 0.7)
    col = m.mix(mul(m, wipe, 0.10), col, lighten(m, c.col, 0.25))
    scr = mul(m, isolines(m, scaled(m, c.sep, 6.0, 6.0, 140.0), 1.0, 0.006, 2.0), ramp(m, noise(m, c.P, scale=6.0, detail=2.0), 0.45, 0.65))
    col = m.mix(mul(m, scr, 0.35), col, H(0xa59e8c))
    scuff = mul(m, ramp(m, c.z, 0.5, 0.08), ramp(m, noise(m, scaled(m, c.sep, 8.0, 8.0, 150.0), detail=4.0), 0.58, 0.72))
    col = m.mix(mul(m, scuff, 0.55), col, H(0x5e584c))
    chips = ramp(m, convex_edges(m, c.tc, distance=0.002, samples=12, breakup_scale=60.0), 0.45, 0.6)
    col = m.mix(chips, col, m.mix(ramp(m, noise(m, c.P, scale=120.0, detail=2.0), 0.4, 0.6), H(0x7b7d7f), H(0x6a3a1e)))
    rough = mix_float(m, chips, add(m, 0.26, mul(m, peel, 0.06), mul(m, wipe, -0.06), mul(m, scuff, 0.18), mul(m, scr, 0.1)), 0.55)
    metal = mix_float(m, chips, 0.0, 0.4)
    return col, rough, metal, add(m, mul(m, peel, 0.012), mul(m, chips, -0.05), mul(m, scr, -0.01))


def f_plastic(c, rough0=0.5):
    m = c.m
    g = noise(m, c.P, scale=1800.0, detail=1.0)
    edges = mul(m, convex_edges(m, c.tc, distance=0.0012, samples=10, breakup_scale=220.0), 0.6)
    col = m.mix(mul(m, edges, 0.5), c.col, lighten(m, c.col, 0.25))
    rough = m.math('SUBTRACT', add(m, rough0, mul(m, g, 0.06)), mul(m, edges, 0.1))
    return col, rough, 0.0, mul(m, g, 0.004)


def f_gloss(c):
    m = c.m
    scratch = ramp(m, noise(m, c.P, scale=900.0, detail=4.0, distortion=2.0), 0.62, 0.7)
    smear = ramp(m, noise(m, c.P, scale=14.0, detail=3.0), 0.5, 0.8)
    return c.col, add(m, 0.1, mul(m, scratch, 0.12), mul(m, smear, 0.12)), 0.0, 0.0


def f_rubber(c):
    m = c.m
    g = noise(m, c.P, scale=900.0, detail=1.0)
    dust = ramp(m, noise(m, c.P, scale=20.0, detail=3.0), 0.5, 0.8)
    col = m.mix(mul(m, dust, 0.35), c.col, H(0x5a5650))
    return col, add(m, 0.8, mul(m, g, 0.08)), 0.0, mul(m, g, 0.01)


def f_kick(c):
    """A kick plate: dark paint, scuffed grey by shoes and carts, black heel marks, a little rust."""
    m = c.m
    streak = noise(m, scaled(m, c.sep, 3.0, 3.0, 80.0), detail=4.0)
    scuff = mul(m, ramp(m, streak, 0.58, 0.72), ramp(m, noise(m, c.P, scale=9.0, detail=3.0), 0.35, 0.65))
    heel = mul(m, ramp(m, noise(m, scaled(m, c.sep, 6.0, 6.0, 120.0), detail=3.0), 0.66, 0.74), 0.9)
    rust = ramp(m, noise(m, c.P, scale=30.0, detail=5.0), 0.7, 0.78)
    col = m.mix(scuff, c.col, H(0x8a8a86))
    col = m.mix(heel, col, H(0x0d0d0d))
    col = m.mix(mul(m, rust, 0.8), col, H(0x5a3018))
    rough = add(m, 0.5, mul(m, scuff, 0.2), mul(m, rust, 0.3))
    metal = mul(m, scuff, 0.5)
    return col, rough, metal, m.math('SUBTRACT', mul(m, rust, 0.05), mul(m, scuff, 0.02))


def f_powder(c):
    m = c.m
    peel = noise(m, c.P, scale=900.0, detail=1.0)
    edge = ramp(m, convex_edges(m, c.tc, distance=0.0015, samples=10, breakup_scale=120.0), 0.45, 0.6)
    col = m.mix(edge, c.col, H(0x8b8e93))
    rough = mix_float(m, edge, add(m, 0.45, mul(m, peel, 0.08)), 0.32)
    return col, rough, mix_float(m, edge, 0.0, 1.0), mul(m, peel, 0.006)


def f_laminate(c, flecks=1.0):
    """High-pressure laminate: a speckled print, worn smooth and lighter where hands slide, chipped at the edge to the
    particle board, stained."""
    m = c.m
    mott = noise(m, c.P, scale=40.0, detail=3.0)
    sp1 = ramp(m, noise(m, c.P, scale=330.0, detail=1.0), 0.68, 0.71)
    sp2 = ramp(m, noise(m, vec(m, c.x, c.y, add(m, c.z, 3.1)), scale=420.0, detail=1.0), 0.3, 0.27)
    sp3 = ramp(m, noise(m, vec(m, c.y, c.x, add(m, c.z, 7.3)), scale=260.0, detail=1.0), 0.7, 0.72)
    col = m.mix(mul(m, mott, 0.25), c.col, darken(m, c.col, 0.9))
    col = m.mix(mul(m, sp1, 0.75 * flecks), col, darken(m, c.col, 0.5))
    col = m.mix(mul(m, sp2, 0.6 * flecks), col, lighten(m, c.col, 0.55))
    col = m.mix(mul(m, sp3, 0.6 * flecks), col, H(0x8a7350))
    worn = ramp(m, noise(m, c.P, scale=1.6, detail=3.0), 0.45, 0.7)
    col = m.mix(mul(m, worn, 0.14), col, lighten(m, c.col, 0.3))
    stain = ramp(m, noise(m, c.P, scale=5.0, detail=5.0, distortion=0.6), 0.68, 0.75)
    col = m.mix(mul(m, stain, 0.4), col, H(0x8a7a5a))
    scr = mul(m, isolines(m, scaled(m, c.sep, 120.0, 9.0, 1.0), 1.0, 0.005, 2.0), ramp(m, noise(m, c.P, scale=4.0, detail=2.0), 0.4, 0.6))
    col = m.mix(mul(m, scr, 0.35), col, darken(m, c.col, 0.7))
    for (rx, ry, rr, rh) in c.kit.rings:
        d = m.math('SQRT', add(m, mul(m, m.math('SUBTRACT', c.x, rx), m.math('SUBTRACT', c.x, rx)), mul(m, m.math('SUBTRACT', c.y, ry), m.math('SUBTRACT', c.y, ry))))
        band = mul(m, ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', d, rr)), 0.0025, 0.0005), ramp(m, noise(m, c.P, scale=60.0, detail=2.0), 0.3, 0.55))
        fill = mul(m, smooth_less(m, d, rr, 0.002), 0.25)
        col = m.mix(mul(m, m.math('MAXIMUM', band, fill), 0.6), col, H(rh))
        stain = m.math('MAXIMUM', stain, band)
    rough = add(m, 0.34, mul(m, worn, -0.1), mul(m, stain, 0.12), mul(m, sp1, 0.04), mul(m, scr, 0.08))
    chip = None
    for (cx, cy, r) in c.kit.laminate_chips:
        d = m.math('SQRT', add(m, mul(m, m.math('SUBTRACT', c.x, cx), m.math('SUBTRACT', c.x, cx)),
                               mul(m, m.math('SUBTRACT', c.y, cy), m.math('SUBTRACT', c.y, cy))))
        wob = mul(m, m.math('SUBTRACT', noise(m, c.P, scale=180.0, detail=2.0), 0.5), r * 0.8)
        k = smooth_less(m, m.math('ADD', d, wob), r, r * 0.08)
        chip = k if chip is None else m.math('MAXIMUM', chip, k)
    h = mul(m, sp1, 0.002)
    if chip is not None:
        board = m.mix(ramp(m, noise(m, c.P, scale=400.0, detail=2.0), 0.4, 0.6), H(0x9c7a50), H(0x6e5232))
        col = m.mix(chip, col, board)
        rough = mix_float(m, chip, rough, 0.9)
        h = m.math('SUBTRACT', h, mul(m, chip, 0.6))
    return col, rough, 0.0, h


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
    fade = ramp(m, noise(m, c.P, scale=3.0, detail=3.0), 0.4, 0.75)
    col = m.mix(mul(m, fade, 0.25), col, H(0xd8b888))
    return col, 0.9, 0.0, add(m, mul(m, gran, 0.4), mul(m, tone, 0.1), mul(m, pins, -0.3))


def f_alu(c):
    m = c.m
    br = noise(m, scaled(m, c.sep, 600.0, 600.0, 6.0), detail=3.0)
    sm = ramp(m, noise(m, c.P, scale=10.0, detail=4.0), 0.5, 0.8)
    col = m.mix(br, H(0xa9acaf), H(0xc4c7ca))
    return m.mix(mul(m, sm, 0.3), col, H(0x7c7f82)), add(m, 0.3, mul(m, br, 0.06), mul(m, sm, 0.12)), 1.0, mul(m, br, 0.003)


def f_cloth(c):
    m = c.m
    period = 0.002
    wa = m.math('SINE', mul(m, add(m, c.x, c.z), 2 * math.pi / period))
    wb = m.math('SINE', mul(m, add(m, c.y, c.z), 2 * math.pi / period))
    weave = mul(m, add(m, mul(m, wa, wb), 1.0), 0.5)
    heather = noise(m, c.P, scale=600.0, detail=2.0)
    fold = ramp(m, noise(m, c.P, scale=30.0, detail=3.0), 0.3, 0.7)
    crease = isolines(m, c.P, 22.0, 0.05, 3.0)
    col = m.mix(mul(m, heather, 0.4), c.col, lighten(m, c.col, 0.12))
    col = m.mix(mul(m, fold, 0.25), col, darken(m, c.col, 0.7))
    col = m.mix(mul(m, crease, 0.3), col, darken(m, c.col, 0.6))
    return col, 0.92, 0.0, add(m, mul(m, weave, 0.08), mul(m, heather, 0.04), mul(m, crease, -0.9), mul(m, fold, 0.6))


def f_paper(c):
    m = c.m
    fib = noise(m, scaled(m, c.sep, 900.0, 300.0, 900.0), detail=2.0)
    col = m.mix(mul(m, fib, 0.15), c.col, darken(m, c.col, 0.85))
    return col, add(m, 0.78, mul(m, fib, 0.1)), 0.0, mul(m, fib, 0.004)


def f_bag(c):
    """A milky plastic bag over folded laundry: the clothes' colors faintly through it, crinkles."""
    m = c.m
    vor = m.node('ShaderNodeTexVoronoi', Scale=45.0)
    vor.feature = 'DISTANCE_TO_EDGE'
    m.link(add_vec(m, c.P, noise_vec(m, c.P, 8.0)), vor.inputs['Vector'])
    crease = ramp(m, vor.outputs['Distance'], 0.04, 0.0)
    band = noise(m, scaled(m, c.sep, 2.0, 2.0, 22.0), detail=2.0)
    inside = m.mix(ramp(m, band, 0.35, 0.65), H(0x3b4f6e), H(0xb8b2a6))
    inside = m.mix(ramp(m, noise(m, c.P, scale=4.0, detail=2.0), 0.6, 0.7), inside, H(0x9a3a32))
    col = m.mix(0.32, c.col, inside)
    col = m.mix(mul(m, crease, 0.4), col, lighten(m, c.col, 0.6))
    return col, add(m, 0.24, mul(m, crease, -0.08), mul(m, band, 0.1)), 0.0, add(m, mul(m, crease, 0.25), mul(m, band, 0.4))


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


def f_drum(c):
    """A washer's perforated stainless drum (axis along Y through the kit's drum center)."""
    m = c.m
    cx, cz, R = c.kit.drum
    dx = m.math('SUBTRACT', c.x, cx)
    dz = m.math('SUBTRACT', c.z, cz)
    r = m.math('SQRT', add(m, mul(m, dx, dx), mul(m, dz, dz)))
    ang = m.math('ARCTAN2', dz, dx)
    on_wall = ramp(m, r, R * 0.93, R * 0.98)
    a = mix_float(m, on_wall, c.x, mul(m, ang, R))
    b = mix_float(m, on_wall, c.z, c.y)
    pitch = 0.012
    ga = m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', a, pitch)), 0.5)
    gb = m.math('SUBTRACT', m.math('FRACT', m.math('DIVIDE', b, pitch)), 0.5)
    d = mul(m, m.math('SQRT', add(m, mul(m, ga, ga), mul(m, gb, gb))), pitch)
    holes = smooth_less(m, d, 0.0024, 0.0004)
    br = noise(m, scaled(m, c.sep, 300.0, 4.0, 300.0), detail=2.0)
    col = m.mix(br, H(0xa3a6a9), H(0xc2c5c8))
    col = m.mix(holes, col, H(0x0b0b0b))
    return col, mix_float(m, holes, add(m, 0.36, mul(m, br, 0.08)), 0.9), mix_float(m, holes, 0.3, 0.0), mul(m, holes, -0.8)


def f_paint(c):
    """Interior paint on steel (a door, its frame): roller stipple, chipped to primer and steel on the edges."""
    m = c.m
    stip = noise(m, c.P, scale=350.0, detail=2.0)
    edge = convex_edges(m, c.tc, distance=0.003, samples=12, breakup_scale=50.0, gain=5.0)
    chip = ramp(m, edge, 0.45, 0.6)
    bare = ramp(m, edge, 0.7, 0.85)
    col = m.mix(mul(m, stip, 0.2), c.col, darken(m, c.col, 0.88))
    col = m.mix(chip, col, H(0x8a8478))
    col = m.mix(bare, col, H(0x6f7275))
    scuff = mul(m, ramp(m, noise(m, scaled(m, c.sep, 4.0, 4.0, 60.0), detail=4.0), 0.56, 0.7), ramp(m, c.z, 0.6, 0.05))
    col = m.mix(mul(m, scuff, 0.8), col, H(0x2e2b27))
    scr = mul(m, isolines(m, scaled(m, c.sep, 8.0, 8.0, 260.0), 1.0, 0.007, 2.0),
              mul(m, ramp(m, noise(m, c.P, scale=5.0, detail=2.0), 0.45, 0.62), ramp(m, c.z, 1.3, 0.9)))
    col = m.mix(mul(m, scr, 0.75), col, H(0xa39d8f))
    roll = noise(m, scaled(m, c.sep, 30.0, 30.0, 2.0), detail=2.0)
    col = m.mix(mul(m, roll, 0.12), col, lighten(m, c.col, 0.2))
    rough = add(m, 0.42, mul(m, stip, 0.1), mul(m, chip, 0.15), mul(m, scuff, 0.15), mul(m, roll, -0.06))
    return col, rough, mul(m, bare, 0.8), add(m, mul(m, stip, 0.01), mul(m, chip, -0.06), mul(m, scr, -0.02))


def f_particle(c):
    m = c.m
    n = noise(m, c.P, scale=400.0, detail=3.0)
    return m.mix(n, H(0x9c7a50), H(0x6e5232)), 0.9, 0.0, mul(m, n, 0.1)


def f_felt(c):
    m = c.m
    n = noise(m, c.P, scale=500.0, detail=4.0)
    return m.mix(mul(m, n, 0.5), c.col, lighten(m, c.col, 0.15)), 0.95, 0.0, mul(m, n, 0.05)


FINISH = {'steel': f_steel, 'chrome': f_chrome, 'enamel': f_enamel, 'plastic': f_plastic, 'gloss': f_gloss, 'rubber': f_rubber, 'kick': f_kick,
          'powder': f_powder, 'laminate': f_laminate, 'cork': f_cork, 'alu': f_alu, 'cloth': f_cloth, 'paper': f_paper, 'bag': f_bag,
          'drum': f_drum, 'paint': f_paint, 'particle': f_particle, 'felt': f_felt, 'solid': lambda c: f_laminate(c, 0.0)}
METAL_FINS = {'steel', 'chrome', 'alu', 'drum'}


def body_material(kit):
    m = core.Mat(f'{kit.prefix}_body')
    c = Ctx(m, kit)
    col, rough, metal, h = H(0x808080), 0.5, 0.0, 0.0
    for kind in sorted(kit.kinds, key=FIN.get):
        fc, fr, fm, fh = FINISH[kind](c)
        mk = is_k(m, c.fin, FIN[kind])
        col = m.mix(mk, col, fc)
        rough = mix_float(m, mk, rough, fr)
        metal = mix_float(m, mk, metal, fm)
        h = mix_float(m, mk, h, fh)
    # Wear every finish shares: grime in the crevices, rising from the floor, hand grime, detergent drips, dust on top.
    ao = m.node('ShaderNodeAmbientOcclusion', Distance=0.012)
    ao.only_local = True
    ao.samples = 8
    cav = ramp(m, ao.outputs['AO'], 0.85, 0.4)
    col = m.mix(mul(m, cav, 0.35), col, H(0x2e2a24))
    rough = add(m, rough, mul(m, cav, 0.08))
    breakup = ramp(m, noise(m, c.P, scale=7.0, detail=4.0), 0.3, 0.7)
    low = mul(m, ramp(m, c.z, kit.low, 0.0), breakup)
    col = m.mix(mul(m, low, 0.5), col, H(0x332e27))
    rough = mix_float(m, mul(m, low, 0.6), rough, 0.85)
    metal = mix_float(m, mul(m, low, 0.5), metal, 0.0)
    for (cx, cy, cz), rad, amt in kit.grime:
        dx, dy, dz = m.math('SUBTRACT', c.x, cx), m.math('SUBTRACT', c.y, cy), m.math('SUBTRACT', c.z, cz)
        d = m.math('SQRT', add(m, mul(m, dx, dx), mul(m, dy, dy), mul(m, dz, dz)))
        g = mul(m, mul(m, smooth_less(m, d, rad * 0.6, rad * 0.4), ramp(m, noise(m, c.P, scale=35.0, detail=3.0), 0.3, 0.7)), amt)
        col = m.mix(mul(m, g, 0.7), col, H(0x332d24))
        rough = mix_float(m, g, rough, 0.62)
    for (cx, cy, cz), rad, amt in kit.polish:
        dx, dy, dz = m.math('SUBTRACT', c.x, cx), m.math('SUBTRACT', c.y, cy), m.math('SUBTRACT', c.z, cz)
        d = m.math('SQRT', add(m, mul(m, dx, dx), mul(m, dy, dy), mul(m, dz, dz)))
        g = mul(m, mul(m, smooth_less(m, d, rad * 0.55, rad * 0.45), ramp(m, noise(m, c.P, scale=18.0, detail=3.0), 0.25, 0.65)), amt)
        col = m.mix(mul(m, g, 0.12), col, darken(m, col, 0.8))
        rough = mix_float(m, g, rough, 0.24)
    for (x0, x1, ztop, length) in kit.drips:
        inx = mul(m, ramp(m, c.x, x0, x0 + 0.01), ramp(m, c.x, x1, x1 - 0.01))
        streak = ramp(m, noise(m, scaled(m, c.sep, 260.0, 260.0, 3.0), detail=2.0), 0.56, 0.66)
        runs = mul(m, ramp(m, c.z, ztop - length, ztop - length * 0.35), ramp(m, c.z, ztop + 0.002, ztop - 0.004))
        front = ramp(m, c.ny, -0.5, -0.8)
        dr = mul(m, mul(m, inx, mul(m, streak, runs)), front)
        col = m.mix(mul(m, dr, 0.75), col, H(0xc9d1d8))
        rough = mix_float(m, dr, rough, 0.6)
        metal = mix_float(m, dr, metal, 0.0)
    if kit.dust > 0.0:
        # Dust settles on what faces up, finer on the hard surfaces people wipe (metals take less of it).
        metal_less = m.math('SUBTRACT', 1.0, mul(m, metal, 0.6)) if not isinstance(metal, (int, float)) else 1.0 - 0.6 * metal
        up = mul(m, ramp(m, c.nz, 0.6, 0.95), mul(m, ramp(m, noise(m, c.P, scale=22.0, detail=4.0), 0.4, 0.8), kit.dust))
        up = mul(m, up, metal_less)
        if 'cloth' in kit.kinds:   # laundry gets handled and shaken out: no dust on it
            up = mul(m, up, m.math('SUBTRACT', 1.0, is_k(m, c.fin, FIN['cloth'])))
        col = m.mix(up, col, H(0x9a958b))
        rough = mix_float(m, up, rough, 0.9)
        metal = mix_float(m, up, metal, 0.0)
    m.set('Base Color', col)
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    bump = m.node('ShaderNodeBump', Strength=1.0, Distance=0.001)
    m.link(h if not isinstance(h, (int, float)) else m.math('ADD', h, 0.0), bump.inputs['Height'])
    m.set('Normal', bump.outputs['Normal'])
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
    col = m.mix(mul(m, grime, 0.16), img.outputs['Color'], H(0x3a342c))
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


# ================================================================== print panels: shared pieces

INK = 0x141414
PAPER = 0xf1eee4


def p_sticker(lines, ground=0xf2c230, ink=INK, title=None, title_ground=INK, title_ink=0xf2c230):
    """A warning sticker: an optional title band, centered lines."""
    def draw(s, w, h):
        s.rect(0, 0, w, h, ground, rough=0.35)
        top = h
        if title:
            bh = h * 0.24
            s.rect(2, h - bh - 2, w - 4, bh, title_ground, rough=0.35)
            s.mid(title, w / 2, h - bh / 2 - 2, bh * 0.6, w - 10, title_ink, 'Black')
            top = h - bh - 4
        n = len(lines)
        lh = min(top / (n + 0.6), w * 0.16)
        for i, ln in enumerate(lines):
            s.mid(ln, w / 2, top - lh * (i + 0.8), lh * 0.62, w - 8, ink, 'Bold')
    return draw


def p_plate(lines, ground=0xb9b6ad, ink=0x2a2a2a, metal=1.0):
    def draw(s, w, h):
        s.rect(0, 0, w, h, ground, metal=metal, rough=0.35)
        for i, ln in enumerate(lines):
            s.mid(ln, w / 2, h - h * (i + 0.7) / (len(lines) + 0.4), h / (len(lines) + 0.4) * 0.5, w - 6, ink, 'Bold', metal=metal, rough=0.4)
        for x in (3.0, w - 3.0):
            for y in (3.0, h - 3.0):
                s.circle(x, y, 1.2, 0x8c8a84, metal=1.0, rough=0.3, n=16)
    return draw


def speed_spin(s, w, h, sub='COMMERCIAL'):
    """The maker's badge: SPEED SPIN in chrome on black, with a swoosh."""
    s.rect(0, 0, w, h, 0x121314, rough=0.3)
    s.rect(1.5, 1.5, w - 3, h - 3, 0x1a1b1d, rough=0.25)
    s.mid('SPEED SPIN', w * 0.5, h * 0.6, h * 0.48, w * 0.82, 0xdcdedf, 'Black', tracking=1.05, metal=0.8, rough=0.38)
    s.mid(sub, w * 0.5, h * 0.2, h * 0.16, w * 0.6, 0xb21f1a, 'Bold', tracking=1.6, rough=0.3)


# ================================================================== SM_WF_Washer

WASH_W, WASH_D, WASH_H = 0.69, 0.75, 1.10
WY0 = -WASH_D / 2          # the front
WDOOR_Z = 0.50
WASHER_NO = 7              # its WF_Number stickers (front and console)
WBUTTONS = [(-0.065, 'HOT', 0xc8372d), (-0.005, 'WARM', 0xd9822a), (0.055, 'COLD', 0x2f6fb5), (0.115, 'DELICATE', 0x7f8b94)]


def draw_washer_fascia(s, w, h):
    """The console's black fascia, 660 x 180 mm; x = (world X + 0.33) * 1000, y = (world Z - 0.90) * 1000."""
    s.rect(0, 0, w, h, 0x17181a, rough=0.5)
    s.rect(2.5, 2.5, w - 5, h - 5, 0x1c1d20, rough=0.45)
    s.rect(196, 14, 1.2, h - 28, 0x34363a, rough=0.4)
    s.rect(500, 14, 1.2, h - 28, 0x34363a, rough=0.4)
    s.mid('SOAP  ·  BLEACH  ·  SOFTENER', 99, 171, 6.0, 170, 0xb9bcc0, 'Bold', tracking=1.1)
    s.mid('ADD SOAP BEFORE YOU PAY', 99, 12, 5.5, 170, 0x8d9196, 'Bold')
    # The machine's number sticker (WF_Number) goes on at (242, 145), r 19.
    s.ring(242, 145, 19.5, 20.6, 0x2c2e31, rough=0.4)
    s.mid('MACHINE', 242, 116, 5.0, 50, 0x8d9196, 'Bold', tracking=1.2)
    s.mid('PRICE', 355, 172, 6.0, 60, 0xb9bcc0, 'Bold', tracking=1.3)
    s.mid('SELECT CYCLE', 355, 100, 7.5, 180, 0xe9e6dc, 'Black', tracking=1.15)
    for x, lab, col in WBUTTONS:
        px = (x + 0.33) * 1000
        s.mid(lab, px, 40, 6.0, 54, 0xe9e6dc, 'Bold')
        s.circle(px, 88, 2.3, 0x47e06a if lab == 'WARM' else 0x2c2e31, rough=0.2, n=20)
    s.mid('QUARTERS', 543, 172, 6.0, 64, 0xb9bcc0, 'Bold', tracking=1.1)
    s.mid('1 LOAD   2 ADD SOAP   3 CLOSE DOOR   4 PAY   5 SELECT', 355, 18, 5.4, 290, 0x9ea2a7, 'Bold')
    s.mid('DOOR LOCKS UNTIL CYCLE ENDS', 355, 8, 4.6, 240, 0x7d8186, 'Bold', tracking=1.1)


def draw_washer_display(s, w, h):
    s.rect(0, 0, w, h, 0x1a0405, rough=0.06)
    s.rect(2, 2, w - 4, h - 4, 0x120203, rough=0.06)
    # ghost segments behind the lit ones, as on a real LED readout
    s.mid('$8.88', w / 2, h / 2, h * 0.72, w * 0.86, 0x3a0a08, 'Bold', rough=0.06)
    s.mid('$3.25', w / 2, h / 2, h * 0.72, w * 0.86, 0xff3a22, 'Bold', rough=0.06)


def draw_washer_drawer(s, w, h):
    s.rect(0, 0, w, h, 0xcdcfcb, rough=0.45)
    for i, (lab, glyph) in enumerate((('PRE', 'I'), ('WASH', 'II'), ('SOFT', '*'))):
        cx = w * (i + 0.5) / 3
        s.ring(cx, h * 0.62, 11, 12.6, 0x2a2c2f, rough=0.4)
        if glyph == '*':
            for k in range(5):
                a = k * 2 * math.pi / 5 + math.pi / 2
                s.circle(cx + 5 * math.cos(a), h * 0.62 + 5 * math.sin(a), 3.6, 0x2a2c2f, n=20)
            s.circle(cx, h * 0.62, 2.5, 0xcdcfcb, n=16)
        else:
            s.mid(glyph, cx, h * 0.62, 11, 18, 0x2a2c2f, 'Black')
        s.mid(lab, cx, h * 0.25, 7, w / 3 - 8, 0x2a2c2f, 'Bold', tracking=1.1)


def draw_coin_label(s, w, h):
    s.rect(0, 0, w, h, 0xd7d8d6, metal=1.0, rough=0.3)
    s.mid('25¢', w / 2, h * 0.62, h * 0.5, w * 0.8, 0x1b1b1b, 'Black', metal=0.0)
    s.mid('QUARTERS ONLY', w / 2, h * 0.2, h * 0.16, w * 0.9, 0x1b1b1b, 'Bold', metal=0.0)


def draw_card_lcd(s, w, h):
    s.rect(0, 0, w, h, 0x0e2318, rough=0.08)
    s.rect(1.5, 1.5, w - 3, h - 3, 0x163a27, rough=0.08)
    s.mid('INSERT CARD', w * 0.46, h * 0.68, h * 0.22, w * 0.8, 0x9cffc0, 'Bold', rough=0.08)
    s.mid('$3.25', w * 0.46, h * 0.3, h * 0.32, w * 0.6, 0xc8ffd8, 'Bold', rough=0.08)
    s.circle(w - 5, h - 5, 1.8, 0x5cff7a, rough=0.1, n=16)


def draw_card_logo(s, w, h):
    s.rect(0, 0, w, h, 0x1d1e20, rough=0.4)
    s.rect(4, h * 0.25, 13, h * 0.5, 0x2f8fd8, rough=0.3)
    s.rect(4, h * 0.55, 13, 1.6, 0x1d1e20, rough=0.3)
    s.mid('SPINCARD', w * 0.62, h * 0.5, h * 0.36, w * 0.6, 0xdfe3e8, 'Black')


def draw_badge(s, w, h):
    speed_spin(s, w, h, 'COMMERCIAL')


def washer():
    k = Kit('wfw', 'SM_WF_Washer', dust=0.12, drum=(0.0, WDOOR_Z, 0.25))
    y0 = WY0
    k.panel('fascia', 0.66, 0.18, draw_washer_fascia)
    k.panel('display', 0.096, 0.032, draw_washer_display, glow=6.0, density=2.0)
    k.panel('drawer', 0.176, 0.10, draw_washer_drawer, density=1.4)
    k.panel('badge', 0.194, 0.044, draw_badge, density=1.6)
    k.panel('caution', 0.09, 0.11, p_sticker(['DOOR LOCKS', 'DURING CYCLE.', 'DO NOT FORCE.', 'WAIT FOR THE', 'DOOR LIGHT.'], title='CAUTION'), density=1.8)
    k.panel('coin', 0.05, 0.03, draw_coin_label, density=2.0)
    k.panel('lcd', 0.05, 0.024, draw_card_lcd, glow=3.0, density=2.5)
    k.panel('cardlogo', 0.056, 0.02, draw_card_logo, density=2.0)
    k.panel('rating', 0.10, 0.03, p_plate(['SPEED SPIN  SW-20  20 LB', '120 V  60 HZ  12 A  HOT 140°F MAX', 'SER 4471-0921']), density=2.0)
    k.atlas.render()

    # The cabinet: sides, back, base pan and lid in stainless; the lower front with the door's hole.
    k.add(bx('side_l', -0.345, -0.337, y0 + 0.004, y0 + WASH_D, 0.02, 1.092, 0.003), 'steel')
    k.add(bx('side_r', 0.337, 0.345, y0 + 0.004, y0 + WASH_D, 0.02, 1.092, 0.003), 'steel')
    k.add(bx('back', -0.337, 0.337, y0 + 0.742, y0 + WASH_D - 0.001, 0.02, 1.09, 0.0), 'steel')
    k.add(bx('pan', -0.337, 0.337, y0 + 0.02, y0 + 0.742, 0.02, 0.03, 0.0), 'kick', 0x3a3b3c)
    k.add(bx('lid', -0.345, 0.345, y0 + 0.19, y0 + WASH_D, 1.078, 1.092, 0.004), 'steel')
    k.add(front_plate('front', core.rounded_rect(-0.345, 0.13, 0.345, 0.885, 0.004, 3), [circle_xz(0.0, WDOOR_Z, 0.205, 96)], y0, 0.008), 'steel')
    # Console: a stainless box over the front, the black fascia set in it, the drawer, the display and buttons.
    k.add(bx('console', -0.345, 0.345, y0 - 0.006, y0 + 0.20, 0.885, 1.10, 0.006, 3), 'steel')
    k.add(bx('fascia', -0.33, 0.33, y0 - 0.010, y0 - 0.004, 0.90, 1.082, 0.002), 'plastic', 0x1b1c1e)
    k.decal('fascia', (0.0, y0 - 0.010, 0.991), w=0.66, h=0.18)
    k.add(bx('drawer', -0.322, -0.140, y0 - 0.026, y0 - 0.009, 0.924, 1.064, 0.004, 3), 'plastic', 0xcfd1cd)
    k.add(bx('drawer_lip', -0.312, -0.150, y0 - 0.036, y0 - 0.020, 0.924, 0.935, 0.004, 3), 'plastic', 0xcfd1cd)
    k.decal('drawer', (-0.231, y0 - 0.026, 1.006))
    k.add(bx('display_bezel', 0.025 - 0.054, 0.025 + 0.054, y0 - 0.016, y0 - 0.009, 1.025, 1.065, 0.002), 'gloss', 0x0d0d0e)
    k.decal('display', (0.025, y0 - 0.016, 1.045))
    for x, lab, col in WBUTTONS:
        k.add(parts.rbox('button', (x, y0 - 0.0135, 0.965), (0.044, 0.009, 0.028), 0.004, 3), 'gloss', col)
    # Coin drop: a chrome plate, the slot, the return button; the card reader beside it.
    k.add(bx('coin_plate', 0.178, 0.248, y0 - 0.024, y0 - 0.009, 0.935, 1.065, 0.004, 3), 'chrome')
    k.add(bx('coin_slot', 0.2095, 0.2165, y0 - 0.0255, y0 - 0.022, 1.014, 1.052, 0.001), 'rubber', 0x050505)
    k.add(parts.rbox('coin_guard', (0.213, y0 - 0.027, 1.056), (0.03, 0.008, 0.006), 0.002), 'chrome')
    k.add(disc_y('coin_return', 0.213, y0 - 0.022, 0.962, 0.012, 0.009, 24), 'chrome')
    k.decal('coin', (0.213, y0 - 0.024, 0.993))
    k.add(bx('reader', 0.256, 0.327, y0 - 0.048, y0 - 0.009, 0.925, 1.074, 0.008, 3), 'plastic', 0x1d1e20)
    k.decal('lcd', (0.2915, y0 - 0.048, 1.046))
    k.add(bx('card_lip', 0.264, 0.319, y0 - 0.058, y0 - 0.046, 0.958, 0.973, 0.003), 'plastic', 0x2b2c2f)
    k.add(bx('card_slit', 0.268, 0.315, y0 - 0.0585, y0 - 0.0555, 0.9645, 0.9665, 0.0), 'rubber', 0x030303)
    k.decal('cardlogo', (0.2915, y0 - 0.048, 1.002))
    # The door: a stainless ring with its black pull, the hinge, the glass bowl reaching into the drum.
    door = at_front(0.0, y0, WDOOR_Z)
    ring = [(0.168, 0.006), (0.200, 0.004), (0.226, 0.005), (0.236, 0.010), (0.240, 0.022), (0.239, 0.036), (0.233, 0.048), (0.220, 0.056),
            (0.200, 0.060), (0.184, 0.059), (0.172, 0.054), (0.166, 0.044), (0.165, 0.030)]
    k.add(lathe_solid('ring', ring, door, 96), 'steel')
    k.add(lathe_solid('ring_inner', [(0.150, 0.012), (0.170, 0.012), (0.170, 0.050), (0.162, 0.050)], door, 96), 'rubber', 0x1a1a1a)
    k.special(lathe_open('glass', [(0.163, 0.048), (0.160, 0.020), (0.150, -0.020), (0.128, -0.060), (0.090, -0.086), (0.045, -0.097), (0.0, -0.100)],
                         door, 96), 'WF_Glass')
    k.add(bx('pull', 0.222, 0.258, y0 - 0.064, y0 - 0.020, 0.43, 0.57, 0.013, 4), 'plastic', 0x1c1d1f)
    k.add(bx('hinge', -0.272, -0.232, y0 - 0.032, y0 - 0.001, 0.44, 0.56, 0.006, 3), 'chrome')
    for zz in (0.455, 0.545):
        k.add(parts.rod('hinge_pin', (-0.262, y0 - 0.034, zz - 0.022), (-0.262, y0 - 0.034, zz + 0.022), 0.006, 16), 'chrome')
    # The boot: rubber folding from the front's hole back to the drum's mouth.
    boot = [(0.205, 0.003), (0.213, -0.004), (0.209, -0.016), (0.196, -0.030), (0.186, -0.044), (0.195, -0.060), (0.192, -0.080), (0.194, -0.100),
            (0.186, -0.100), (0.184, -0.080), (0.187, -0.060), (0.178, -0.044), (0.188, -0.030), (0.200, -0.016), (0.204, -0.006), (0.198, 0.003)]
    k.add(lathe_solid('boot', boot, door, 96), 'rubber', 0x1b1b1b)
    # The drum: perforated stainless, three lifters.
    k.add(lathe_open('drum', [(0.19, -0.098), (0.25, -0.104), (0.25, -0.53), (0.0, -0.53)], door, 96), 'drum')
    for a in (90.0, 210.0, 330.0):
        th = math.radians(a)
        er = Vector((math.cos(th), math.sin(th), 0.0))
        et = Vector((-math.sin(th), math.cos(th), 0.0))
        loc = Matrix((et, -er, Vector((0, 0, 1)))).transposed().to_4x4()
        loc.translation = er * 0.248
        lifter = parts.panel('lifter', [(-0.034, 0.0), (0.034, 0.0), (0.012, 0.042), (-0.012, 0.042)], -0.51, -0.12, r=0.006, matrix=door @ loc)
        k.add(lifter, 'drum')
    # Badge, number, caution sticker, rating plate.
    k.add(bx('badge_plate', -0.100, 0.100, y0 - 0.004, y0 + 0.001, 0.795, 0.845, 0.002), 'chrome')
    k.decal('badge', (0.0, y0 - 0.004, 0.820))
    num_decal(k, WASHER_NO, (-0.29, y0, 0.832), 0.07, lift=0.0005)
    num_decal(k, WASHER_NO, (0.242 - 0.33, y0 - 0.010, 1.045), 0.038, lift=0.0009)
    k.decal('caution', (0.29, y0, 0.818))
    k.decal('rating', (0.0, y0, 0.200))
    # Kick plate and feet.
    k.add(bx('kick', -0.335, 0.335, y0 + 0.012, y0 + 0.022, 0.022, 0.128, 0.002), 'kick', 0x2e2f30)
    for sx in (-1, 1):
        for yy in (y0 + 0.05, y0 + 0.70):
            k.add(parts.rod('foot', (sx * 0.30, yy, 0.0), (sx * 0.30, yy, 0.012), 0.02, 20), 'rubber', 0x202020)
            k.add(parts.rod('stem', (sx * 0.30, yy, 0.012), (sx * 0.30, yy, 0.024), 0.008, 12), 'chrome')
    k.grime = [((0.24, y0 - 0.04, WDOOR_Z), 0.07, 1.0), ((-0.23, y0 - 0.03, 0.93), 0.06, 0.8), ((0.03, y0 - 0.012, 0.965), 0.09, 0.5),
               ((0.0, y0, 0.27), 0.12, 0.5)]
    k.drips = [(-0.31, -0.15, 0.924, 0.07), (-0.30, -0.16, 0.885, 0.16)]
    return [k.finish()]


# ================================================================== SM_WF_DryerStack

DRY_W, DRY_D, DRY_H = 0.79, 0.80, 1.95
DY0 = -DRY_D / 2
DRUMS_Z = (0.50, 1.42)     # the lower drum, the upper
OPEN_R = 0.262
BAND = (0.85, 1.07)        # the control band's print (Z)
HALVES = ((-0.20, 'UPPER DRYER', 1), (0.20, 'LOWER DRYER', 0))   # (center X, label, which drum)
DRYER_NO = (32, 31)        # WF_Number stickers: the lower drum, the upper (31 over 32)


def draw_dryer_band(s, w, h):
    """770 x 220 mm; x = (world X + 0.385) * 1000, y = (world Z - 0.85) * 1000."""
    s.rect(0, 0, w, h, 0x232426, rough=0.5)
    s.rect(3, 3, w - 6, h - 6, 0x292a2d, rough=0.45)
    s.rect(w / 2 - 0.8, 16, 1.6, h - 32, 0x46484c, rough=0.4)
    for cxw, label, upper in HALVES:
        px = (cxw + 0.385) * 1000
        s.text(label, px - 168, 200, 8.5, 0xe6e2d6, 'Black', tracking=1.12)
        ax = px - 168 + text_w(s.s, label, 'Black', 1.12) * 8.5 + 8
        if upper:
            s.poly([(ax, 199), (ax + 10, 199), (ax + 5, 208)], 0xf0c94a)
        else:
            s.poly([(ax, 208), (ax + 10, 208), (ax + 5, 199)], 0xf0c94a)
        s.mid('25¢ = 6 MIN', px - 130, 36, 7.5, 84, 0xf0c94a, 'Black')
        s.mid('QUARTERS ONLY', px - 130, 23, 4.8, 84, 0xb9bcc0, 'Bold', tracking=1.1)
        s.mid('TIME LEFT', px + 10, 197, 5.0, 70, 0x9ea2a7, 'Bold', tracking=1.2)
        s.mid('TEMPERATURE', px + 5, 137, 5.5, 150, 0x9ea2a7, 'Bold', tracking=1.2)
        for bxx, lab in ((px - 50, 'HIGH'), (px + 5, 'MED'), (px + 60, 'LOW')):
            s.mid(lab, bxx, 88, 5.6, 46, 0xe6e2d6, 'Bold')
        s.mid('PUSH TO', px + 140, 88, 5.6, 70, 0xe6e2d6, 'Black')
        s.mid('START', px + 140, 79, 5.6, 70, 0xe6e2d6, 'Black')
        s.ring(px + 140, 125, 29, 30.5, 0x46484c, rough=0.4)
    s.mid('NO REFUNDS  ·  CLEAN LINT SCREEN BEFORE EACH LOAD  ·  DO NOT OVERLOAD', w / 2, 9, 4.8, w - 60, 0x8d9196, 'Bold')


def p_led(text):
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0x1a0405, rough=0.06)
        s.rect(1.5, 1.5, w - 3, h - 3, 0x120203, rough=0.06)
        s.mid('88', w / 2, h / 2, h * 0.74, w * 0.8, 0x3a0a08, 'Bold', rough=0.06)
        s.mid(text, w / 2, h / 2, h * 0.74, w * 0.8, 0xff3a22, 'Bold', rough=0.06)
    return draw


def draw_dryer_logo(s, w, h):
    s.rect(0, 0, w, h, 0xbdbab1, metal=1.0, rough=0.32)
    s.rect(2, 2, w - 4, h - 4, 0xc9c6bd, metal=1.0, rough=0.28)
    s.mid('SPEED', w * 0.30, h * 0.58, h * 0.42, w * 0.38, 0x1b2c4a, 'Black', tracking=1.12, metal=0.0, rough=0.35)
    s.mid('SPIN', w * 0.72, h * 0.58, h * 0.42, w * 0.34, 0x9e1f1a, 'Black', tracking=1.3, metal=0.0, rough=0.35)
    s.mid('COMMERCIAL TUMBLE DRYER  ·  30 LB  ·  STACK', w / 2, h * 0.18, h * 0.13, w * 0.86, 0x3a3a3a, 'Bold', metal=0.0, rough=0.4)
    for x in (4.0, w - 4.0):
        for y in (4.0, h - 4.0):
            s.circle(x, y, 1.6, 0x8c8a84, metal=1.0, rough=0.3, n=16)


def dryer_drum(k, M):
    """The drum (WF_DrumGlow): DRUM_PROFILE turned, its lathe UVs squeezed into v 0..DRUM_V; three ribs in the strip
    above."""
    obj = core.lathe('drum', DRUM_PROFILE, 96)
    uv = obj.data.uv_layers['UVMap']
    for d in uv.data:
        d.uv = (d.uv.x, d.uv.y * DRUM_V)
    core.orient_normals(obj, (0.0, 0.0, 1.0))
    obj.data.transform(M)
    k.special(obj, 'WF_DrumGlow', locked=True)
    z0, z1 = DRUM_PROFILE[-1][1] + 0.01, DRUM_PROFILE[1][1] - 0.01
    for i, a in enumerate((90.0, 210.0, 330.0)):
        th = math.radians(a)
        er = Vector((math.cos(th), math.sin(th), 0.0))
        et = Vector((-math.sin(th), math.cos(th), 0.0))
        loc = Matrix((et, -er, Vector((0, 0, 1)))).transposed().to_4x4()
        loc.translation = er * (DRUM_R - 0.002)
        rib = parts.panel('rib', [(-0.055, 0.0), (0.055, 0.0), (0.022, 0.075), (-0.022, 0.075)], z0, z1, r=0.01)
        me = rib.data
        ruv = me.uv_layers.get('UVMap') or me.uv_layers.new(name='UVMap')
        for poly in me.polygons:
            for li in poly.loop_indices:
                co = me.vertices[me.loops[li].vertex_index].co
                ruv.data[li].uv = ((i + (co.z - z0) / (z1 - z0)) / 3.0, DRUM_V + 0.015 + (1 - DRUM_V - 0.03) * (0.5 + co.x / 0.12))
        rib.data.transform(M @ loc)
        k.special(rib, 'WF_DrumGlow', locked=True)


def laundry(k, cx, cy, axis_z, radius, rnd):
    """A forgotten load lying in the bottom of a drum (axis along Y at height axis_z): soft lumps kept inside it."""
    pieces = [((0.30, 0.26, 0.11), 0xd9d3c4), ((0.24, 0.30, 0.09), 0x3b5373), ((0.20, 0.22, 0.08), 0x9e2e2a), ((0.26, 0.20, 0.07), 0x7d7f84),
              ((0.16, 0.18, 0.06), 0x2f6b4f)]
    for i, (size, col) in enumerate(pieces):
        x = cx + rnd.uniform(-0.12, 0.12)
        y = cy + rnd.uniform(-0.14, 0.14)
        z = axis_z - radius + size[2] * 0.45 + rnd.uniform(0.0, 0.03) + 0.02 * (i % 2)
        b = blob('laundry', (x, y, z), size, seed=11 + i, detail=0.3, subdiv=4, flatten=0.6, wrinkle=0.24, smooth=1)
        for v in b.data.vertices:
            dx, dz = v.co.x, v.co.z - axis_z
            r = math.hypot(dx, dz)
            if r > radius - 0.006:
                f = (radius - 0.006) / r
                v.co.x, v.co.z = dx * f, axis_z + dz * f
        k.add(b, 'cloth', col)


def dryer():
    k = Kit('wfd', 'SM_WF_DryerStack', dust=0.7)
    y0 = DY0
    k.panel('band', 0.77, 0.22, draw_dryer_band)
    k.panel('led_u', 0.072, 0.026, p_led('00'), glow=5.0, density=2.0)
    k.panel('led_l', 0.072, 0.026, p_led('25¢'), glow=5.0, density=2.0)
    k.panel('logo', 0.27, 0.055, draw_dryer_logo, density=1.5)
    k.panel('lint', 0.20, 0.075, p_sticker(['BEFORE EACH LOAD', 'LINT IS A FIRE HAZARD'], title='CLEAN LINT SCREEN'), density=1.5)
    k.panel('coin', 0.05, 0.03, draw_coin_label, density=2.0)
    k.atlas.render()
    rnd = random.Random(31)
    almond = 0xd6ccb2
    # The cabinet in almond enamel: sides, back, lid; two fronts with the doors' holes; the control band between.
    k.add(bx('side_l', -0.395, -0.387, y0 + 0.004, y0 + DRY_D, 0.035, 1.935, 0.003), 'enamel', almond)
    k.add(bx('side_r', 0.387, 0.395, y0 + 0.004, y0 + DRY_D, 0.035, 1.935, 0.003), 'enamel', almond)
    k.add(bx('back', -0.387, 0.387, y0 + 0.792, y0 + DRY_D - 0.001, 0.035, 1.935, 0.0), 'enamel', almond)
    k.add(bx('lid', -0.395, 0.395, y0 + 0.0, y0 + DRY_D, 1.935, 1.95, 0.004, 3), 'enamel', almond)
    k.add(front_plate('front_lo', core.rounded_rect(-0.395, 0.035, 0.395, 0.840, 0.003, 3), [circle_xz(0.0, DRUMS_Z[0], OPEN_R, 96)], y0, 0.008),
          'enamel', almond)
    k.add(front_plate('front_hi', core.rounded_rect(-0.395, 1.080, 0.395, 1.935, 0.003, 3), [circle_xz(0.0, DRUMS_Z[1], OPEN_R, 96)], y0, 0.008),
          'enamel', almond)
    k.add(bx('band', -0.395, 0.395, y0 - 0.012, y0 + 0.10, 0.842, 1.078, 0.004, 3), 'powder', 0x2a2b2d)
    k.decal('band', (0.0, y0 - 0.012, sum(BAND) / 2), w=0.77, h=0.22)
    # The toe kick, the lint door (louvers, a keyed lock) and its sticker.
    k.add(bx('toe', -0.385, 0.385, y0 + 0.012, y0 + 0.024, 0.0, 0.036, 0.002), 'kick', 0x29292a)
    k.add(bx('lint_door', -0.30, 0.30, y0 - 0.006, y0 + 0.001, 0.045, 0.165, 0.003, 3), 'steel')
    for i in range(5):
        z = 0.064 + i * 0.019
        k.add(bx('louver', -0.035, 0.235, y0 - 0.0105, y0 - 0.005, z, z + 0.008, 0.002), 'steel')
    k.add(disc_y('lock', 0.268, y0 - 0.005, 0.105, 0.014, 0.006, 32), 'chrome')
    k.add(bx('keyway', 0.2665, 0.2695, y0 - 0.0115, y0 - 0.010, 0.0975, 0.1125, 0.0), 'rubber', 0x050505)
    k.decal('lint', (-0.165, y0 - 0.006, 0.105))
    for zc in DRUMS_Z:
        M = at_front(0.0, y0, zc)
        bezel = [(0.252, 0.016), (0.256, 0.006), (0.290, 0.004), (0.306, 0.008), (0.312, 0.020), (0.310, 0.034), (0.300, 0.044), (0.282, 0.050),
                 (0.264, 0.050), (0.253, 0.044), (0.250, 0.030)]
        k.add(lathe_solid('bezel', bezel, M, 96), 'chrome')
        k.add(lathe_solid('door_frame', [(0.226, 0.030), (0.255, 0.030), (0.255, 0.046), (0.234, 0.047)], M, 96), 'plastic', 0x1e1f21)
        k.special(lathe_open('glass', [(0.238, 0.044), (0.200, 0.050), (0.120, 0.055), (0.0, 0.057)], M, 96), 'WF_Glass')
        k.add(lathe_solid('seal', [(0.262, 0.001), (0.267, -0.010), (0.259, -0.040), (0.250, -0.046), (0.247, -0.040), (0.252, -0.010), (0.256, 0.001)], M, 96),
              'felt', 0x3b3a38)
        dryer_drum(k, M)
        pts = core.catmull_rom([(0.266, y0 - 0.045, zc - 0.10), (0.288, y0 - 0.075, zc - 0.072), (0.297, y0 - 0.083, zc), (0.288, y0 - 0.075, zc + 0.072),
                                (0.266, y0 - 0.045, zc + 0.10)], 6)
        k.add(tube('handle', pts, 0.0095, 14), 'chrome')
        for dz in (-0.10, 0.10):
            k.add(parts.rod('handle_post', (0.266, y0 - 0.036, zc + dz), (0.266, y0 - 0.05, zc + dz), 0.012, 16), 'chrome')
        k.add(bx('hinge', -0.338, -0.296, y0 - 0.032, y0 - 0.001, zc - 0.12, zc + 0.12, 0.006, 3), 'chrome')
        for dz in (-0.07, 0.07):
            k.add(parts.rod('hinge_pin', (-0.326, y0 - 0.034, zc + dz - 0.025), (-0.326, y0 - 0.034, zc + dz + 0.025), 0.007, 16), 'chrome')
    # The controls on the band, per drum.
    for cxw, label, upper in HALVES:
        cx = cxw - 0.13
        k.add(bx('coin_plate', cx - 0.031, cx + 0.031, y0 - 0.030, y0 - 0.011, 0.902, 1.012, 0.004, 3), 'chrome')
        k.add(bx('coin_slot', cx - 0.0035, cx + 0.0035, y0 - 0.0315, y0 - 0.028, 0.968, 1.004, 0.001), 'rubber', 0x050505)
        k.add(disc_y('coin_knob', cx, y0 - 0.029, 0.922, 0.013, 0.014, 24), 'chrome')
        k.decal('coin', (cx, y0 - 0.030, 0.951), w=0.05, h=0.026)
        lx = cxw + 0.01
        k.add(bx('led_bezel', lx - 0.042, lx + 0.042, y0 - 0.019, y0 - 0.011, 1.008, 1.042, 0.002), 'gloss', 0x0d0d0e)
        k.decal('led_u' if upper else 'led_l', (lx, y0 - 0.019, 1.025))
        for dx, col in ((-0.05, 0xc8372d), (0.005, 0xd9822a), (0.06, 0x2f6fb5)):
            k.add(parts.rbox('temp', (cxw + dx, y0 - 0.0155, 0.96), (0.044, 0.009, 0.024), 0.004, 3), 'gloss', col)
        sx = cxw + 0.14
        k.add(disc_y('start_ring', sx, y0 - 0.011, 0.975, 0.027, 0.008, 40), 'chrome')
        k.add(disc_y('start', sx, y0 - 0.017, 0.975, 0.021, 0.008, 40), 'gloss', 0x2e9a4a)
    # The maker's plate, the machine numbers.
    k.add(bx('logo_plate', -0.138, 0.138, y0 - 0.004, y0 + 0.001, 1.8355, 1.8945, 0.003), 'chrome')
    k.decal('logo', (0.0, y0 - 0.004, 1.865))
    num_decal(k, DRYER_NO[1], (-0.33, y0, 1.69), 0.075, lift=0.0005)
    num_decal(k, DRYER_NO[0], (-0.33, y0, 0.765), 0.075, lift=0.0005)
    # The forgotten load in the upper drum.
    laundry(k, 0.0, y0 + 0.36, DRUMS_Z[1], DRUM_R, rnd)
    k.grime = [((0.29, y0 - 0.07, DRUMS_Z[0]), 0.08, 1.0), ((0.29, y0 - 0.07, DRUMS_Z[1]), 0.08, 1.0), ((-0.33, y0 - 0.02, 0.96), 0.06, 0.6),
               ((0.07, y0 - 0.02, 0.96), 0.06, 0.6), ((0.27, y0, 0.105), 0.05, 0.6), ((0.0, y0, 0.20), 0.15, 0.4)]
    return [k.finish()]


# ================================================================== SM_WF_FoldTable

TABLE_Z = 0.898


def draw_chip(s, w, h):
    r = w / 2
    s.circle(r, r, r, 0xa3201c, rough=0.45, n=64)
    for i in range(8):
        a = i * math.pi / 4
        cx, cy = r + math.cos(a) * r * 0.86, r + math.sin(a) * r * 0.86
        s.poly([(cx + math.cos(a + d) * r * 0.16, cy + math.sin(a + d) * r * 0.16) for d in (1.2, math.pi - 1.2, math.pi + 1.2, -1.2)], 0xf1ede2, rough=0.45)
    s.circle(r, r, r * 0.6, 0xe9e2d0, rough=0.4, n=48)
    s.ring(r, r, r * 0.52, r * 0.56, 0xa3201c, rough=0.4)
    s.mid('5', r, r, r * 0.62, r * 0.6, 0xa3201c, 'Black')


def draw_nosit(s, w, h):
    s.rect(0, 0, w, h, 0xe9e2c8, rough=0.5)
    s.rect(2, 2, w - 4, h - 4, 0xc23a2c, rough=0.5)
    s.rect(4, 4, w - 8, h - 8, 0xe9e2c8, rough=0.5)
    s.mid('NO SITTING ON TABLES', w / 2, h * 0.62, h * 0.3, w - 16, 0xb02a20, 'Black')
    s.mid('FOLDING ONLY · THANK YOU', w / 2, h * 0.25, h * 0.16, w - 30, 0x3a3630, 'Bold')


def sock(k, path, col_body, col_heel, col_stripe, heel_t=0.48, seed=5):
    """A lost crew sock lying flat: a soft tube along the path (the leg, the heel's pocket, the foot), the toe rounded
    shut, rumpled; heel and toe darker, two stripes at the cuff."""
    pts = [Vector(p) for p in core.catmull_rom(path, 3)]
    n = len(pts)
    L = [0.0]
    for a, b in zip(pts, pts[1:]):
        L.append(L[-1] + (b - a).length)
    Z = Vector((0.0, 0.0, 1.0))
    rings = []
    for i, P in enumerate(pts):
        t = L[i] / L[-1]
        tan = pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]
        tan.z = 0.0
        tan.normalize()
        side = Z.cross(tan).normalized()
        hw = 0.042 + 0.008 * math.exp(-((t - heel_t) / 0.07) ** 2)
        ht = 0.009 + 0.007 * math.exp(-((t - heel_t) / 0.06) ** 2)
        if t > 0.86:
            f = math.sqrt(max(0.04, 1.0 - ((t - 0.86) / 0.14) ** 2))
            hw *= f
            ht *= max(f, 0.3)
        ring = []
        for j in range(16):
            q = 2 * math.pi * j / 16
            cq, sq = math.cos(q), math.sin(q)
            x = hw * math.copysign(abs(cq) ** 0.7, cq)
            z = ht * (math.copysign(abs(sq) ** 0.9, sq) + 1.0)
            ring.append(tuple(P + side * x + Z * z))
        rings.append(ring)
    obj = core.loft('sock', rings, caps=True)
    core.subdivide(obj, 1)
    me = obj.data
    co = np.array([v.co[:] for v in me.vertices])
    n1 = texmaps.value_noise(co, 0.035, seed=seed, octaves=2) - 0.5
    n2 = texmaps.value_noise(co, 0.012, seed=seed + 1, octaves=1) - 0.5
    for v, a, b in zip(me.vertices, n1, n2):
        up = max(0.0, min(1.0, (v.co.z - TABLE_Z - 0.004) / 0.01))
        v.co.z = max(TABLE_Z + 0.0004, v.co.z + up * (0.009 * a + 0.003 * b))
        v.co.x += 0.004 * b + 0.003 * a
        v.co.y += 0.003 * a
    me.shade_smooth()
    k.add(obj, 'cloth', col_body)
    P2 = np.array([[p.x, p.y] for p in pts])
    C2 = np.array([[p.center.x, p.center.y] for p in me.polygons])
    _, along = texmaps.polyline_distance(C2, P2)
    cb, ch, cs = lin3(col_body), lin3(col_heel), lin3(col_stripe)
    vals = []
    for a in along:
        a *= 1.0
        if abs(a - heel_t) < 0.07 or a > 0.9:
            vals.extend(ch)
        elif 0.03 < a < 0.05 or 0.07 < a < 0.09:
            vals.extend(cs)
        else:
            vals.extend(cb)
    me.attributes['wf_col'].data.foreach_set('vector', vals)


def draw_lost_tape(s, w, h):
    s.rect(0, 0, w, h, 0xe6d9ab, rough=0.7)
    s.rect(0, 0, w, 1.6, 0xd4c697, rough=0.7)
    s.rect(0, h - 1.6, w, 1.6, 0xd4c697, rough=0.7)
    size = h * 0.62
    x = (w - hand_w('LOST + FOUND', size)) / 2
    hand(s, 'LOST + FOUND', x, h * 0.2, size, 0x111111, width=0.12, slant=0.06, seed=12)


def lost_and_found(k, cx, cy, z0, rnd):
    """A cardboard box of what people left behind, on the table's lower shelf: flaps flopped open, a hoodie and a kid's
    shirt in it, a sleeve over the side; LOST + FOUND in marker on masking tape."""
    W, D, Hb, t = 0.42, 0.30, 0.22, 0.004
    kraft = 0xa8875a
    k.add(bx('box_bottom', cx - W / 2, cx + W / 2, cy - D / 2, cy + D / 2, z0, z0 + t), 'paper', kraft)
    for sy in (-1, 1):
        y0, y1 = sorted((cy + sy * D / 2, cy + sy * (D / 2 - t)))
        k.add(bx('box_wall', cx - W / 2, cx + W / 2, y0, y1, z0, z0 + Hb), 'paper', kraft)
    for sx in (-1, 1):
        x0, x1 = sorted((cx + sx * W / 2, cx + sx * (W / 2 - t)))
        k.add(bx('box_end', x0, x1, cy - D / 2 + t, cy + D / 2 - t, z0, z0 + Hb), 'paper', kraft)
    for sy, ang in ((-1, 62.0), (1, -96.0)):
        flap = bx('flap', -W / 2, W / 2, -t / 2, t / 2, 0.0, D / 2 - 0.004)
        flap.data.transform(Matrix.Translation((cx, cy + sy * (D / 2 - t / 2), z0 + Hb)) @ Matrix.Rotation(math.radians(ang), 4, 'X'))
        k.add(flap, 'paper', kraft)
    for sx, ang in ((-1, -98.0), (1, 104.0)):
        flap = bx('flap_end', -t / 2, t / 2, -D / 2 + 0.006, D / 2 - 0.006, 0.0, 0.10)
        flap.data.transform(Matrix.Translation((cx + sx * (W / 2 - t / 2), cy, z0 + Hb)) @ Matrix.Rotation(math.radians(ang), 4, 'Y'))
        k.add(flap, 'paper', kraft)
    k.decal('lost', (cx - 0.02, cy - D / 2, z0 + Hb * 0.58), lift=0.0005)
    # What's in it, kept inside the walls.
    for i, (dx, dy, dz, size, col) in enumerate(((-0.05, 0.0, Hb - 0.03, (0.36, 0.26, 0.16), 0x6f7378), (0.08, 0.03, Hb + 0.005, (0.22, 0.18, 0.07), 0xe0b52c),
                                                 (-0.11, -0.04, Hb + 0.012, (0.16, 0.14, 0.05), 0x2f4f7f))):
        b = blob('lost_cloth', (cx + dx, cy + dy, z0 + dz), size, seed=60 + i, detail=0.22, subdiv=4 if i == 0 else 3, flatten=0.3, wrinkle=0.2, smooth=1)
        for v in b.data.vertices:
            v.co.x = min(max(v.co.x, cx - W / 2 + t + 0.003), cx + W / 2 - t - 0.003)
            v.co.y = min(max(v.co.y, cy - D / 2 + t + 0.003), cy + D / 2 - t - 0.003)
            v.co.z = max(v.co.z, z0 + t + 0.002)
        k.add(b, 'cloth', col)
    sleeve = core.catmull_rom([(cx + 0.06, cy - 0.06, z0 + Hb - 0.01), (cx + 0.08, cy - D / 2 + 0.01, z0 + Hb + 0.016), (cx + 0.09, cy - D / 2 - 0.016, z0 + Hb + 0.005),
                               (cx + 0.10, cy - D / 2 - 0.022, z0 + Hb - 0.06), (cx + 0.11, cy - D / 2 - 0.02, z0 + Hb - 0.10)], 5)
    k.add(core.sweep('sleeve', sleeve, [(0.012 * math.cos(2 * math.pi * j / 12), 0.034 * math.sin(2 * math.pi * j / 12)) for j in range(12)]), 'cloth', 0x6f7378)


def fold_table():
    k = Kit('wft', 'SM_WF_FoldTable', dust=0.15)
    k.panel('chip', 0.039, 0.039, draw_chip, density=3.0)
    k.panel('nosit', 0.20, 0.055, draw_nosit, density=1.5)
    k.panel('lost', 0.30, 0.055, draw_lost_tape, density=1.6)
    k.atlas.render()
    rnd = random.Random(240)
    z1 = TABLE_Z
    grey = 0x5b5f64
    # The top: laminate over particle board, a black T-molding round it, chips out of the laminate's edge.
    k.add(parts.panel('top', core.rounded_rect(-1.196, -0.446, 1.196, 0.446, 0.035, 8), z1 - 0.030, z1, r=0.0012, segments=2), 'laminate', 0xc6bca3)
    prof = core.rounded_rect(-0.0175, -0.0022, 0.0175, 0.0022, 0.0015, 2)
    k.add(core.sweep('molding', rrect_path(-1.198, -0.448, 1.198, 0.448, 0.037, z1 - 0.0145, 8), prof, closed=True, caps=False), 'rubber', 0x1c1c1c)
    k.laminate_chips = [(1.168, -0.428, 0.016), (-0.64, -0.432, 0.011), (0.33, 0.433, 0.013), (-1.17, 0.40, 0.009)]
    k.rings = [(-0.42, 0.18, 0.038, 0x6e5a40), (0.10, -0.25, 0.052, 0x9db5cc), (-0.36, 0.215, 0.036, 0x7a6448)]
    # The apron, six legs on glides, the lower shelf on its stretchers.
    for sy in (-1, 1):
        k.add(bx('apron', -1.15, 1.15, sy * 0.405 - 0.0025, sy * 0.405 + 0.0025, z1 - 0.066, z1 - 0.030, 0.0008), 'powder', grey)
        k.add(bx('flange', -1.15, 1.15, min(sy * 0.405, sy * 0.38), max(sy * 0.405, sy * 0.38), z1 - 0.034, z1 - 0.030, 0.0005), 'powder', grey)
    for sx in (-1, 1):
        k.add(bx('apron_end', sx * 1.15 - 0.0025, sx * 1.15 + 0.0025, -0.4025, 0.4025, z1 - 0.066, z1 - 0.030, 0.0008), 'powder', grey)
    for x in (-1.13, 0.0, 1.13):
        for y in (-0.38, 0.38):
            k.add(bx('leg', x - 0.019, x + 0.019, y - 0.019, y + 0.019, 0.022, z1 - 0.034, 0.003), 'powder', grey)
            k.add(parts.rod('glide', (x, y, 0.0), (x, y, 0.022), 0.018, 20), 'rubber', 0x222222)
    for sy in (-1, 1):
        k.add(bx('stretcher', -1.111, 1.111, sy * 0.38 - 0.012, sy * 0.38 + 0.012, 0.212, 0.236, 0.002), 'powder', grey)
    for sx in (-1, 1):
        k.add(bx('stretcher_end', sx * 1.13 - 0.012, sx * 1.13 + 0.012, -0.361, 0.361, 0.212, 0.236, 0.002), 'powder', grey)
    k.add(bx('shelf', -1.118, 1.118, -0.372, 0.372, 0.236, 0.255, 0.002), 'laminate', 0xb9af98)
    lost_and_found(k, -0.62, -0.02, 0.255, rnd)
    # The lost sock, two red $5 chips from the back room, the sticker.
    path = [(0.58, 0.125, z1), (0.64, 0.102, z1), (0.70, 0.078, z1), (0.745, 0.052, z1), (0.775, 0.018, z1), (0.79, -0.03, z1), (0.80, -0.08, z1),
            (0.806, -0.13, z1), (0.80, -0.175, z1)]
    sock(k, path, 0x8f9197, 0x34363c, 0xa3302a)
    chip_prof = [(0.0, 0.0), (0.0186, 0.0), (0.0194, 0.0005), (0.0197, 0.0016), (0.0194, 0.0028), (0.0186, 0.0033), (0.0, 0.0033)]
    circ = [(0.5 + 0.5 * math.cos(2 * math.pi * i / 40), 0.5 + 0.5 * math.sin(2 * math.pi * i / 40)) for i in range(40)]
    for (x, y, z, rz) in ((0.36, -0.21, z1, 0.3), (0.362, -0.211, z1 + 0.0034, 1.4), (0.43, -0.16, z1, 2.2)):
        M = Matrix.Translation((x, y, z)) @ Matrix.Rotation(rz, 4, 'Z')
        k.add(lathe_axis('chip', chip_prof, M, 40), 'plastic', 0xa3201c)
        k.decal('chip', (0.0, 0.0, 0.0034), u=(1, 0, 0), v=(0, 1, 0), w=0.0374, h=0.0374, lift=0.0002, outline=circ, matrix=M)
    k.decal('nosit', (-0.93, 0.33, z1), u=(1, 0, 0), v=(0, 1, 0), lift=0.0003, nu=6, nv=2,
            disp=lambda s, t: 0.004 * max(0.0, (s - 0.8) / 0.2) ** 2 * max(0.0, (t - 0.3) / 0.7))
    k.grime = [((0.0, -0.40, z1), 0.35, 0.5), ((-0.6, -0.40, z1), 0.3, 0.4), ((0.7, -0.40, z1), 0.3, 0.4)]
    return [k.finish()]


# ================================================================== SM_WF_Chairs

CHAIR_X = (-0.795, -0.265, 0.265, 0.795)
CHAIR_COLS = (0xc65a1c, 0xbd561d, 0x24727c, 0xca6020)


def apply_mod(obj, mod):
    core.select_only([obj])
    bpy.ops.object.modifier_apply(modifier=mod.name)


def chair_shell(cx):
    """A molded polypropylene shell: the seat dished, the back reclined and wrapping, rolled lips; 5.5 mm thick."""
    ctrl = [(-0.240, 0.424), (-0.233, 0.447), (-0.208, 0.458), (-0.12, 0.454), (-0.02, 0.446), (0.08, 0.440), (0.142, 0.447), (0.178, 0.470),
            (0.198, 0.530), (0.212, 0.620), (0.228, 0.710), (0.243, 0.790), (0.251, 0.832), (0.246, 0.848)]
    prof = core.catmull_rom(ctrl, 2)
    L = [0.0]
    for a, b in zip(prof, prof[1:]):
        L.append(L[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
    nu = 10
    bm = bmesh.new()
    grid = []
    for (py, pz), ln in zip(prof, L):
        t = ln / L[-1]
        hw = 0.228 - 0.016 * float(texmaps.smoothstep(0.45, 1.0, t))
        if t > 0.9:
            hw *= math.sqrt(max(0.0, 1.0 - ((t - 0.9) / 0.1) ** 2 * 0.55))
        if t < 0.05:
            hw *= 1.0 - 0.12 * ((0.05 - t) / 0.05) ** 2
        seat = 1.0 - float(texmaps.smoothstep(0.42, 0.56, t))
        back = float(texmaps.smoothstep(0.5, 0.62, t))
        row = []
        for i in range(nu + 1):
            u = -1.0 + 2.0 * i / nu
            x = cx + u * hw
            y = py - back * 0.026 * u * u
            z = pz + seat * (0.016 * u * u - 0.012 * float(texmaps.smoothstep(0.86, 1.0, abs(u))))
            row.append(bm.verts.new((x, y, z)))
        grid.append(row)
    for j in range(len(grid) - 1):
        for i in range(nu):
            bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
    obj = core.mesh_object('shell', bm)
    mod = obj.modifiers.new('solid', 'SOLIDIFY')
    mod.thickness = 0.0055
    mod.offset = -1.0
    apply_mod(obj, mod)
    core.subdivide(obj, 1)
    obj.data.shade_smooth()
    return obj


def shell_underside(shell, x, y):
    """Height of the shell's underside above (x, y): a ray up from the floor."""
    from mathutils.bvhtree import BVHTree
    bm = bmesh.new()
    bm.from_mesh(shell.data)
    tree = BVHTree.FromBMesh(bm)
    hit = tree.ray_cast(Vector((x, y, 0.0)), Vector((0.0, 0.0, 1.0)), 2.0)
    bm.free()
    return hit[0].z if hit[0] is not None else 0.44


def chairs():
    k = Kit('wfc', 'SM_WF_Chairs', dust=0.05)
    char = 0x2b2d30
    for cx, col in zip(CHAIR_X, CHAIR_COLS):
        shell = k.add(chair_shell(cx), 'plastic', col)
        # the bracket: a plate under the seat on standoffs (each up into a boss molded under the shell), a pedestal
        # down to the beam
        k.add(bx('plate', cx - 0.15, cx + 0.15, -0.13, 0.10, 0.404, 0.410, 0.002), 'powder', char)
        for dx in (-0.12, 0.12):
            for yy in (-0.10, 0.07):
                top = shell_underside(shell, cx + dx, yy)
                k.add(parts.rod('standoff', (cx + dx, yy, 0.409), (cx + dx, yy, top - 0.004), 0.0065, 12), 'powder', char)
                k.add(parts.rod('boss', (cx + dx, yy, top - 0.006), (cx + dx, yy, top + 0.002), 0.011, 16), 'plastic', col)
        k.add(bx('pedestal', cx - 0.035, cx + 0.035, -0.06, 0.04, 0.378, 0.405, 0.003), 'powder', char)
        k.polish += [((cx, -0.07, 0.45), 0.17, 0.8), ((cx, 0.19, 0.66), 0.12, 0.6)]
    k.add(bx('beam', -1.08, 1.08, -0.035, 0.015, 0.30, 0.38, 0.004, 3), 'powder', char)
    k.add(bx('cap_l', -1.086, -1.08, -0.034, 0.014, 0.301, 0.379, 0.003), 'plastic', 0x121212)
    k.add(bx('cap_r', 1.08, 1.086, -0.034, 0.014, 0.301, 0.379, 0.003), 'plastic', 0x121212)
    for lx in (-0.53, 0.53):
        k.add(bx('post', lx - 0.025, lx + 0.025, -0.035, 0.015, 0.03, 0.30, 0.003), 'powder', char)
        k.add(bx('foot', lx - 0.025, lx + 0.025, -0.27, 0.25, 0.012, 0.042, 0.004), 'powder', char)
        for yy in (-0.255, 0.235):
            k.add(parts.rod('glide', (lx, yy, 0.0), (lx, yy, 0.012), 0.02, 20), 'rubber', 0x1c1c1c)
    k.grime = [((cx, -0.22, 0.45), 0.08, 0.6) for cx in CHAIR_X]
    return [k.finish(40.0)]


# ================================================================== SM_WF_ChangeMachine

def draw_marquee(s, w, h):
    """The lit marquee's acrylic: white, CHANGE in red, the bills it takes in blue."""
    s.rect(0, 0, w, h, 0xf6f3ea, rough=0.2)
    s.rect(6, 6, w - 12, h - 12, 0xfbf8ef, rough=0.2)
    s.mid('CHANGE', w / 2, h * 0.6, h * 0.42, w - 40, 0xc4161c, 'Black', tracking=1.08, rough=0.2)
    s.rect(18, h * 0.27, w - 36, 2.2, 0x1f3f8f, rough=0.2)
    # Bills -> QUARTERS: the arrow is drawn (Roboto has no arrow glyph; it printed as a box).
    a, b, size, gap, aw = '$1  ·  $5  ·  $10  ·  $20', 'QUARTERS', h * 0.11, 9.0, 22.0
    wa, wb = text_w(s.s, a, 'Black') * size, text_w(s.s, b, 'Black') * size
    x0 = (w - wa - wb - 2 * gap - aw) / 2
    cy = h * 0.15
    s.mid(a, x0, cy, size, wa + 1, 0x1f3f8f, 'Black', align='LEFT', rough=0.2)
    ax = x0 + wa + gap
    s.rect(ax, cy - 1.3, aw - 7, 2.6, 0x1f3f8f, rough=0.2)
    s.poly([(ax + aw - 9, cy + 5.0), (ax + aw, cy), (ax + aw - 9, cy - 5.0)], 0x1f3f8f, rough=0.2)
    s.mid(b, ax + aw + gap, cy, size, wb + 1, 0x1f3f8f, 'Black', align='LEFT', rough=0.2)


def draw_marquee_side(s, w, h):
    s.rect(0, 0, w, h, 0xf6f3ea, rough=0.2)
    s.mid('CHANGE', w / 2, h * 0.55, h * 0.38, w - 30, 0xc4161c, 'Black', tracking=1.08, rough=0.2)
    s.mid('QUARTERS', w / 2, h * 0.17, h * 0.12, w - 40, 0x1f3f8f, 'Black', tracking=1.3, rough=0.2)


def draw_change_title(s, w, h):
    s.rect(0, 0, w, h, 0x1d232d, rough=0.5)
    s.rect(0, h - 34, w, 34, 0xc4161c, rough=0.45)
    s.mid('BILL CHANGER', w / 2, h - 17, 20, w - 30, 0xffffff, 'Black', tracking=1.15)
    lines = ['1  INSERT BILL FACE UP, ANY DIRECTION', '2  COLLECT YOUR QUARTERS BELOW', '3  USE QUARTERS IN ANY MACHINE']
    for i, ln in enumerate(lines):
        s.text(ln, 18, h - 62 - i * 22, 10, 0xe8e6df, 'Bold')
    s.rect(18, 14, w - 36, 1.4, 0x4a5262)
    s.text('THIS MACHINE GIVES QUARTERS ONLY  ·  NO REFUNDS', 18, 22, 7.5, 0xf0c94a, 'Bold')


def draw_bill_led(s, w, h):
    s.rect(0, 0, w, h, 0x0a0b0c, rough=0.1)
    for i in range(3):
        x = w * (0.22 + i * 0.28)
        s.poly([(x - 7, h * 0.72), (x + 7, h * 0.72), (x, h * 0.28)], 0x39ff6a, rough=0.1)


def draw_bills(s, w, h):
    s.rect(0, 0, w, h, 0x1d232d, rough=0.5)
    s.mid('BILLS ACCEPTED', w / 2, h - 10, 8, w - 20, 0xe8e6df, 'Black', tracking=1.15)
    for i, d in enumerate(('1', '5', '10', '20')):
        x0 = 12 + i * (w - 24) / 4
        bw = (w - 24) / 4 - 10
        s.rect(x0, 10, bw, 44, 0x9fb59a, rough=0.5)
        s.rect(x0 + 2, 12, bw - 4, 40, 0xc5d3b8, rough=0.5)
        s.circle(x0 + bw / 2, 32, 12, 0x8ea488, rough=0.5)
        s.mid(d, x0 + 10, 44, 8, 16, 0x2f4a2c, 'Black')
        s.mid(d, x0 + bw - 10, 20, 8, 16, 0x2f4a2c, 'Black')
        s.mid('$' + d, x0 + bw / 2, 32, 10, bw * 0.6, 0x2f4a2c, 'Black')


def p_label(text, ground=0x1d232d, ink=0xe8e6df, face='Black', tracking=1.15):
    def draw(s, w, h):
        s.rect(0, 0, w, h, ground, rough=0.5)
        s.mid(text, w / 2, h / 2, h * 0.55, w - 8, ink, face, tracking=tracking)
    return draw


def draw_note(s, w, h):
    """A sheet torn off a pad, in black marker: NO $50s."""
    s.rect(0, 0, w, h, 0xf3efdc, rough=0.8)
    for i in range(5):
        s.rect(0, 12 + i * 14, w, 0.6, 0xa9c4d8, rough=0.8)
    s.rect(14, 0, 0.7, h, 0xd99a9a, rough=0.8)
    size = 30.0
    x = (w - hand_w('NO $50s', size)) / 2 + 6
    hand(s, 'NO $50s', x, h * 0.36, size, 0x141414, width=0.085, seed=7)
    s.stroke([(x, h * 0.25), (x + hand_w('NO $50s', size) * 0.5, h * 0.22), (x + hand_w('NO $50s', size) - 6, h * 0.24)], 2.2, 0x141414)


def draw_tape(s, w, h):
    s.rect(0, 0, w, h, 0xe8dcb0, rough=0.7)
    s.rect(0, 0, 2.0, h, 0xd8cca0, rough=0.7)
    s.rect(w - 2.0, 0, 2.0, h, 0xd8cca0, rough=0.7)


def tape(k, center, angle_deg, normal_axis='y', lift=0.0009, w=0.045):
    a = math.radians(angle_deg)
    if normal_axis == 'y':
        u, v = (math.cos(a), 0.0, math.sin(a)), (-math.sin(a), 0.0, math.cos(a))
    else:
        u, v = (math.cos(a), math.sin(a), 0.0), (-math.sin(a), math.cos(a), 0.0)
    k.decal('tape', center, u=u, v=v, w=w, h=0.016, lift=lift)


def change_machine():
    k = Kit('wfx', 'SM_WF_ChangeMachine', dust=0.4)
    k.panel('marquee', 0.44, 0.16, draw_marquee, glow=4.0)
    k.panel('side', 0.33, 0.15, draw_marquee_side, glow=4.0)
    k.panel('title', 0.40, 0.15, draw_change_title)
    k.panel('bill_led', 0.08, 0.03, draw_bill_led, glow=5.0, density=2.0)
    k.panel('bills', 0.30, 0.08, draw_bills)
    k.panel('cup', 0.20, 0.03, p_label('QUARTERS'), density=1.5)
    k.panel('hint', 0.09, 0.016, p_label('INSERT BILL FACE UP', 0x18191b), density=2.0)
    k.panel('service', 0.16, 0.05, p_sticker(['SEE ATTENDANT FOR', 'SERVICE OR REFUNDS'], ground=0xf1eee4), density=1.6)
    k.panel('note', 0.12, 0.08, draw_note, density=1.5)
    k.panel('tape', 0.045, 0.016, draw_tape, density=1.2)
    k.atlas.render()
    slate, black = 0x2b3442, 0x16181b
    # The pedestal: a base plate, a column, the head plate.
    k.add(bx('base', -0.23, 0.23, -0.20, 0.20, 0.0, 0.012, 0.004), 'powder', black)
    k.add(bx('column', -0.10, 0.10, -0.09, 0.09, 0.012, 0.66, 0.006), 'powder', black)
    k.add(bx('head', -0.20, 0.20, -0.16, 0.16, 0.66, 0.672, 0.003), 'powder', black)
    for sx in (-1, 1):
        for sy in (-1, 1):
            k.add(parts.rod('anchor', (sx * 0.19, sy * 0.16, 0.012), (sx * 0.19, sy * 0.16, 0.022), 0.009, 6), 'chrome')
    # The cabinet and its door, the lit marquee (front and sides) in a black frame.
    k.add(bx('cabinet', -0.23, 0.23, -0.18, 0.18, 0.672, 1.60, 0.006, 3), 'powder', slate)
    k.add(bx('door', -0.215, 0.215, -0.187, -0.179, 0.69, 1.585, 0.003), 'powder', slate)
    k.add(bx('marquee', -0.24, 0.24, -0.20, 0.17, 1.60, 1.80, 0.008, 3), 'powder', black)
    for (x0, x1, z0, z1) in ((-0.24, 0.24, 1.776, 1.80), (-0.24, 0.24, 1.60, 1.624), (-0.24, -0.222, 1.60, 1.80), (0.222, 0.24, 1.60, 1.80)):
        k.add(bx('lip', x0, x1, -0.207, -0.199, z0, z1, 0.002), 'powder', black)
    k.decal('marquee', (0.0, -0.2002, 1.70))
    k.decal('side', (-0.2402, -0.015, 1.70), u=(0, -1, 0), v=(0, 0, 1))
    k.decal('side', (0.2402, -0.015, 1.70), u=(0, 1, 0), v=(0, 0, 1))
    k.decal('title', (0.0, -0.187, 1.48))
    # The bill acceptor: a black bezel, a chrome face, the slot's lip, the green arrows.
    k.add(bx('bezel', -0.065, 0.065, -0.222, -0.186, 1.19, 1.36, 0.012, 3), 'plastic', 0x18191b)
    k.add(bx('face', -0.055, 0.055, -0.2255, -0.220, 1.20, 1.35, 0.004), 'chrome')
    k.add(bx('lip', -0.048, 0.048, -0.250, -0.224, 1.235, 1.256, 0.004, 3), 'plastic', 0x202124)
    k.add(bx('slit', -0.042, 0.042, -0.2505, -0.2485, 1.2445, 1.2465, 0.0), 'rubber', 0x030303)
    k.decal('bill_led', (0.0, -0.2255, 1.30))
    k.decal('hint', (0.0, -0.2255, 1.215))
    k.decal('bills', (0.0, -0.187, 1.11))
    k.decal('service', (-0.06, -0.187, 1.005))
    # The coin cup: a stainless hood over a tray, a few quarters left in it.
    k.add(bx('cup_back', -0.09, 0.09, -0.190, -0.186, 0.76, 0.90, 0.002), 'steel')
    k.add(bx('cup_top', -0.09, 0.09, -0.262, -0.186, 0.886, 0.90, 0.004), 'steel')
    k.add(bx('cup_side_l', -0.09, -0.078, -0.262, -0.186, 0.76, 0.90, 0.003), 'steel')
    k.add(bx('cup_side_r', 0.078, 0.09, -0.262, -0.186, 0.76, 0.90, 0.003), 'steel')
    k.add(bx('cup_tray', -0.08, 0.08, -0.258, -0.186, 0.76, 0.774, 0.003), 'steel')
    k.add(bx('cup_lip', -0.09, 0.09, -0.268, -0.256, 0.76, 0.80, 0.004, 3), 'steel')
    for (x, y, rz, tilt) in ((-0.035, -0.222, 0.0, 0.0), (0.012, -0.236, 0.0, 0.0), (0.04, -0.212, 0.0, 0.0), (-0.01, -0.205, 0.0, 0.35)):
        M = Matrix.Translation((x, y, 0.774 + 0.0009 + (0.009 if tilt else 0.0))) @ Matrix.Rotation(tilt, 4, 'X')
        q = parts.rod('quarter', (0.0, 0.0, -0.00087), (0.0, 0.0, 0.00087), 0.0121, 28)
        q.data.transform(M)
        k.add(q, 'steel')
    k.decal('cup', (0.0, -0.187, 0.93))
    # The T-handle lock.
    k.add(disc_y('escutcheon', 0.172, -0.187, 1.00, 0.022, 0.006, 32), 'chrome')
    k.add(parts.rod('stem', (0.172, -0.192, 1.0), (0.172, -0.214, 1.0), 0.006, 16), 'chrome')
    k.add(bx('t_bar', 0.160, 0.184, -0.224, -0.211, 0.955, 1.045, 0.006, 3), 'chrome')
    # The taped note.
    a = math.radians(4.0)
    k.decal('note', (-0.135, -0.187, 1.30), u=(math.cos(a), 0, math.sin(a)), v=(-math.sin(a), 0, math.cos(a)), lift=0.0005, nu=3, nv=2,
            disp=lambda s, t: 0.003 * max(0.0, (0.35 - t) / 0.35) ** 2)
    tape(k, (-0.188, -0.187, 1.345), 40.0)
    tape(k, (-0.082, -0.187, 1.345), -35.0)
    k.grime = [((0.0, -0.24, 1.25), 0.10, 0.9), ((0.0, -0.24, 0.80), 0.10, 0.8), ((0.172, -0.21, 1.0), 0.06, 0.8), ((0.0, -0.2, 0.70), 0.15, 0.4)]
    return [k.finish()]


# ================================================================== SM_WF_SoapVend

VEND_BRANDS = [('SUDZ', 0x1f5fbf, 0xffd23a, 'ORIGINAL'), ('BRITE', 0xe8681c, 0xffffff, 'OXY BOOST'), ('FRESH', 0x2f9a4a, 0xffffff, 'SPRING RAIN'),
               ('BLEACH', 0xf0efe8, 0x1f3f9f, 'REGULAR'), ('SOFTY', 0xa54594, 0xffffff, 'SOFTENER')]
VEND_PRICES = ['$1.25', '$1.25', '$1.00', '$1.00', '$0.75']
VEND_COLS = (-0.276, -0.178, -0.080, 0.018, 0.116)


def p_box(name, ground, ink, sub):
    def draw(s, w, h):
        s.rect(0, 0, w, h, ground, rough=0.45)
        s.rect(0, 0, w, h * 0.22, ink if name != 'BLEACH' else 0xc4161c, rough=0.45)
        s.mid(name, w * 0.42, h * 0.62, h * 0.5, w * 0.62, ink, 'Black', tracking=1.05)
        s.mid('1 LOAD', w * 0.42, h * 0.11, h * 0.14, w * 0.5, ground if name != 'BLEACH' else 0xffffff, 'Black')
        s.circle(w * 0.86, h * 0.58, h * 0.28, ink, rough=0.45, n=32)
        box_icon(s, name, w * 0.86, h * 0.58, h * 0.22, ground)
        s.mid(sub, w * 0.42, h * 0.31, h * 0.11, w * 0.62, ink, 'Bold')
    return draw


def box_icon(s, name, cx, cy, r, col):
    if name == 'SUDZ':
        for dx, dy, rr in ((-0.35, -0.25, 0.42), (0.38, 0.05, 0.32), (-0.05, 0.45, 0.26)):
            s.ring(cx + dx * r, cy + dy * r, rr * r * 0.62, rr * r, col, n=28)
    elif name == 'BRITE':
        s.poly([(cx + math.cos(i * math.pi / 5 + math.pi / 2) * r * (1.0 if i % 2 == 0 else 0.42),
                 cy + math.sin(i * math.pi / 5 + math.pi / 2) * r * (1.0 if i % 2 == 0 else 0.42)) for i in range(10)], col)
    elif name == 'FRESH':
        s.poly([(cx + math.cos(a) * r * 0.45 - math.sin(a) * 0.0, cy + math.sin(a) * r * 0.9) for a in [i * 2 * math.pi / 24 for i in range(24)]], col)
        s.rect(cx - r * 0.04, cy - r * 0.9, r * 0.08, r * 1.6, 0xffffff if col != 0xffffff else 0x2f9a4a)
    elif name == 'BLEACH':
        s.circle(cx, cy - r * 0.25, r * 0.55, col, n=28)
        s.poly([(cx - r * 0.52, cy - r * 0.08), (cx + r * 0.52, cy - r * 0.08), (cx, cy + r * 0.95)], col)
    else:
        for dx, dy, rr in ((-0.45, -0.15, 0.42), (0.0, 0.2, 0.55), (0.48, -0.12, 0.40)):
            s.circle(cx + dx * r, cy + dy * r, rr * r, col, n=24)
        s.rect(cx - r * 0.85, cy - r * 0.55, r * 1.7, r * 0.42, col)


def draw_prices(s, w, h):
    s.rect(0, 0, w, h, 0xf3f1ea, rough=0.4)
    s.rect(0, h - 4, w, 4, 0x1e4f8f, rough=0.4)
    for i, (cx, p, b) in enumerate(zip(VEND_COLS, VEND_PRICES, VEND_BRANDS)):
        px = (cx + 0.325) * 1000
        s.mid(p, px, h * 0.55, h * 0.48, 80, 0x141414, 'Black')
        s.mid(b[0], px, h * 0.16, h * 0.17, 80, 0x4a4a4a, 'Bold')
        if i:
            s.rect(px - 49, 4, 0.8, h - 10, 0xc9c6bd)


def draw_vend_header(s, w, h):
    s.rect(0, 0, w, h, 0x1e4f8f, rough=0.2)
    s.rect(0, h * 0.30, w, 2.5, 0xffd23a, rough=0.2)
    s.mid('LAUNDRY SUPPLIES', w / 2, h * 0.64, h * 0.34, w - 60, 0xffffff, 'Black', tracking=1.1, rough=0.2)
    s.mid('DETERGENT  ·  BLEACH  ·  SOFTENER  ·  SINGLE LOADS', w / 2, h * 0.15, h * 0.1, w - 80, 0xffd23a, 'Black', tracking=1.1, rough=0.2)


def draw_pay(s, w, h):
    s.rect(0, 0, w, h, 0x1e4f8f, rough=0.45)
    s.mid('PAY HERE', w / 2, h * 0.84, h * 0.13, w - 10, 0xffd23a, 'Black', tracking=1.1)
    for i, ln in enumerate(('1  INSERT COINS', '   OR $1 BILLS', '2  PULL KNOB', '   UNDER ITEM', '3  TAKE ITEM', '   FROM BIN')):
        s.text(ln, 7, h * 0.66 - i * h * 0.11, h * 0.085, 0xffffff, 'Bold')


def draw_push(s, w, h):
    s.rect(0, 0, w, h, 0x26282c, rough=0.3)
    s.mid('PUSH', w / 2, h / 2, h * 0.5, w * 0.4, 0xd6d6d0, 'Black', tracking=1.4, rough=0.3)


def draw_tube(s, w, h):
    s.rect(0, 0, w, h, 0xf4f8ff, rough=0.2)


def quad(name, corners):
    bm = bmesh.new()
    vs = [bm.verts.new(c) for c in corners]
    bm.faces.new(vs)
    return core.mesh_object(name, bm)


def soap_vend():
    k = Kit('wfv', 'SM_WF_SoapVend', dust=0.4)
    for i, (n, g, ink, sub) in enumerate(VEND_BRANDS):
        k.panel(f'b{i}', 0.084, 0.029, p_box(n, g, ink, sub), density=2.0)
    k.panel('prices', 0.49, 0.04, draw_prices, density=1.3)
    k.panel('header', 0.66, 0.18, draw_vend_header, glow=4.0)
    k.panel('pay', 0.11, 0.10, draw_pay, density=1.5)
    k.panel('push', 0.30, 0.06, draw_push)
    k.panel('tube', 0.46, 0.012, draw_tube, glow=8.0)
    k.panel('bill_led', 0.06, 0.02, draw_bill_led, glow=5.0, density=2.0)
    k.panel('coin', 0.05, 0.03, draw_coin_label, density=2.0)
    k.atlas.render()
    rnd = random.Random(212)
    white, blue = 0xe2e1da, 0x1e4f8f
    yf = -0.214   # the door's face
    # The cabinet shell, the kick, the door with its window and the delivery bin.
    k.add(bx('kick', -0.35, 0.35, -0.195, 0.21, 0.0, 0.08, 0.002), 'kick', 0x1d1e1f)
    k.add(bx('side_l', -0.36, -0.35, -0.20, 0.22, 0.08, 1.72, 0.003), 'enamel', white)
    k.add(bx('side_r', 0.35, 0.36, -0.20, 0.22, 0.08, 1.72, 0.003), 'enamel', white)
    k.add(bx('top', -0.36, 0.36, -0.20, 0.22, 1.705, 1.72, 0.004), 'enamel', white)
    k.add(bx('back', -0.35, 0.35, 0.21, 0.22, 0.08, 1.705, 0.0), 'enamel', white)
    k.add(bx('bottom', -0.35, 0.35, -0.20, 0.21, 0.08, 0.10, 0.0), 'enamel', white)
    win = (-0.325, 0.62, 0.165, 1.42)
    binr = (-0.30, 0.20, 0.14, 0.40)

    def rect_pts(r):
        return [(r[0], r[1]), (r[2], r[1]), (r[2], r[3]), (r[0], r[3])]
    k.add(front_plate('door', core.rounded_rect(-0.355, 0.10, 0.355, 1.70, 0.006, 3), [rect_pts(win), rect_pts(binr)], yf, 0.014), 'powder', blue)
    k.add(front_plate('win_trim', rect_pts((win[0] - 0.012, win[1] - 0.012, win[2] + 0.012, win[3] + 0.012)), [rect_pts(win)], yf - 0.006, 0.006), 'chrome')
    k.special(quad('glass', [(win[0], -0.207, win[1]), (win[2], -0.207, win[1]), (win[2], -0.207, win[3]), (win[0], -0.207, win[3])]), 'WF_Glass')
    # The inside: a white back, steel floor, ceiling and walls, the column dividers, the tube.
    k.add(bx('in_back', -0.33, 0.17, 0.10, 0.11, 0.60, 1.44), 'enamel', 0xeceae4)
    k.add(bx('in_floor', -0.33, 0.17, -0.20, 0.11, 0.59, 0.605), 'steel')
    k.add(bx('in_ceiling', -0.33, 0.17, -0.20, 0.11, 1.435, 1.45), 'steel')
    k.add(bx('in_wall_l', -0.336, -0.325, -0.20, 0.11, 0.59, 1.45), 'steel')
    k.add(bx('in_wall_r', 0.165, 0.176, -0.20, 0.11, 0.59, 1.45), 'steel')
    for x in (-0.227, -0.129, -0.031, 0.067):
        k.add(bx('divider', x - 0.0015, x + 0.0015, -0.19, 0.10, 0.605, 1.435), 'steel')
    k.add(bx('tube_box', -0.32, 0.16, -0.200, -0.184, 1.41, 1.435, 0.002), 'steel')
    k.decal('tube', (-0.08, -0.192, 1.41), u=(1, 0, 0), v=(0, -1, 0), lift=0.0005)
    # The stacks: single-load boxes lying flat, a little askew.
    for i, cx in enumerate(VEND_COLS):
        n = rnd.randint(15, 24)
        z = 0.6055
        col = VEND_BRANDS[i][1]
        for j in range(n):
            M = Matrix.Translation((cx + rnd.uniform(-0.003, 0.003), -0.12 + rnd.uniform(-0.004, 0.004), z + 0.0145)) @ Matrix.Rotation(
                math.radians(rnd.uniform(-1.8, 1.8)), 4, 'Z')
            b = parts.rbox('box', (0.0, 0.0, 0.0), (0.084, 0.10, 0.029), 0.0015, 1)
            b.data.transform(M)
            k.add(b, 'paper', col)
            k.decal(f'b{i}', (0.0, -0.05, 0.0), lift=0.0003, matrix=M)
            z += 0.0295
    # Under the window: the price strip, the pull knobs.
    k.decal('prices', (-0.08, yf, 0.585))
    for cx in VEND_COLS:
        k.add(disc_y('escutcheon', cx, yf, 0.52, 0.026, 0.004, 32), 'chrome')
        k.add(parts.rod('plunger', (cx, yf - 0.004, 0.52), (cx, yf - 0.05, 0.52), 0.007, 16), 'chrome')
        k.add(lathe_axis('knob', [(0.0, 0.0), (0.016, 0.0), (0.021, 0.006), (0.022, 0.016), (0.019, 0.024), (0.0, 0.026)], at_front(cx, yf - 0.048, 0.52), 32),
              'gloss', 0x141414)
    # The delivery bin: a dark recess, the PUSH flap.
    k.add(bx('bin_back', binr[0], binr[2], -0.08, -0.07, binr[1], binr[3]), 'plastic', 0x1c1d1f)
    k.add(bx('bin_floor', binr[0], binr[2], -0.20, -0.07, binr[1] - 0.005, binr[1] + 0.005), 'plastic', 0x1c1d1f)
    k.add(bx('bin_top', binr[0], binr[2], -0.20, -0.07, binr[3] - 0.005, binr[3] + 0.005), 'plastic', 0x1c1d1f)
    for x in (binr[0], binr[2]):
        k.add(bx('bin_side', x - 0.005, x + 0.005, -0.20, -0.07, binr[1], binr[3]), 'plastic', 0x1c1d1f)
    flap = bx('flap', binr[0] + 0.006, binr[2] - 0.006, -0.0035, 0.0035, -0.17, 0.0, 0.003)
    Mf = Matrix.Translation((0.0, -0.188, binr[3] - 0.008)) @ Matrix.Rotation(math.radians(14.0), 4, 'X')
    flap.data.transform(Mf)
    k.add(flap, 'gloss', 0x2a2c30)
    k.decal('push', (-0.08, -0.0036, -0.09), lift=0.0003, matrix=Mf)
    # The pay strip: coins, bills, the coin return.
    k.add(bx('coin_plate', 0.225, 0.295, yf - 0.014, yf, 1.24, 1.36, 0.004, 3), 'chrome')
    k.add(bx('coin_slot', 0.2565, 0.2635, yf - 0.0155, yf - 0.012, 1.30, 1.345, 0.001), 'rubber', 0x050505)
    k.add(disc_y('coin_return', 0.26, yf - 0.012, 1.262, 0.011, 0.008, 24), 'chrome')
    k.decal('coin', (0.26, yf - 0.014, 1.285), w=0.05, h=0.024)
    k.add(bx('bill_bezel', 0.205, 0.315, yf - 0.031, yf, 1.02, 1.18, 0.010, 3), 'plastic', 0x18191b)
    k.add(bx('bill_lip', 0.215, 0.305, yf - 0.048, yf - 0.029, 1.09, 1.106, 0.004, 3), 'plastic', 0x202124)
    k.add(bx('bill_slit', 0.219, 0.301, yf - 0.0485, yf - 0.0465, 1.097, 1.099, 0.0), 'rubber', 0x030303)
    k.decal('bill_led', (0.26, yf - 0.031, 1.14))
    k.decal('pay', (0.26, yf, 0.92))
    k.add(bx('return_cup', 0.22, 0.30, yf - 0.03, yf, 0.74, 0.80, 0.004, 3), 'chrome')
    k.add(bx('return_hole', 0.232, 0.288, yf - 0.0305, yf - 0.02, 0.752, 0.788, 0.002), 'rubber', 0x070707)
    k.add(disc_y('lock', 0.30, yf, 0.50, 0.016, 0.006, 24), 'chrome')
    # The lit header in its trim.
    k.decal('header', (0.0, yf, 1.57))
    k.add(front_plate('header_trim', rect_pts((-0.34, 1.47, 0.34, 1.67)), [rect_pts((-0.33, 1.48, 0.33, 1.66))], yf - 0.006, 0.006), 'chrome')
    k.grime = [((-0.08, yf - 0.04, 0.52), 0.25, 0.7), ((0.26, yf - 0.02, 1.2), 0.1, 0.8), ((-0.08, yf, 0.30), 0.2, 0.6)]
    return [k.finish()]


# ================================================================== SM_WF_Cart

def cart():
    k = Kit('wfk', 'SM_WF_Cart', dust=0.1, low_grime=0.08)
    zf = 0.16          # the base frame's center line
    z0, z1 = 0.20, 0.62   # the basket
    bx0, by0 = 0.335, 0.235
    k.add(tube('frame', rrect_path(-0.36, -0.24, 0.36, 0.24, 0.05, zf, 6), 0.0125, 14, closed=True), 'chrome')
    for x in (-0.30, 0.30):
        k.add(parts.rod('cross', (x, -0.235, zf), (x, 0.235, zf), 0.010, 12), 'chrome')
        for y in (-0.20, 0.20):
            k.add(parts.rod('post', (x, y, zf), (x, y, z0), 0.006, 10), 'chrome')
    # Casters, each turned its own way, and the corner bumpers.
    for (x, y, yaw) in ((-0.32, -0.20, 15.0), (0.32, -0.20, 80.0), (-0.32, 0.20, -30.0), (0.32, 0.20, 190.0)):
        k.add(bx('caster_plate', x - 0.032, x + 0.032, y - 0.032, y + 0.032, zf - 0.0215, zf - 0.0125, 0.003), 'chrome')
        k.add(parts.rod('swivel', (x, y, zf - 0.034), (x, y, zf - 0.0215), 0.018, 20), 'chrome')
        R = Matrix.Translation((x, y, 0.0)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z')
        fk = []
        for sx in (-1, 1):
            fk.append(bx('fork', sx * 0.0155 - 0.0018, sx * 0.0155 + 0.0018, -0.006, 0.040, 0.035, zf - 0.034, 0.0015))
        wheel = parts.disc('wheel', (0.0, 0.022, 0.0375), (1, 0, 0), 0.0375, 0.024, 28)
        hub = parts.disc('hub', (0.0, 0.022, 0.0375), (1, 0, 0), 0.016, 0.033, 20)
        for o in fk + [hub]:
            o.data.transform(R)
            k.add(o, 'chrome')
        wheel.data.transform(R)
        k.add(wheel, 'rubber', 0x55585b)
    for sx in (-1, 1):
        for sy in (-1, 1):
            k.add(parts.rod('bumper', (sx * 0.345, sy * 0.225, zf + 0.012), (sx * 0.345, sy * 0.225, zf + 0.036), 0.04, 24), 'rubber', 0x6a6d70)
    # The basket: rims and a band, verticals every 45 mm, the floor's wires.
    for z, r in ((z1, 0.004), ((z0 + z1) / 2, 0.0025), (z0, 0.003)):
        k.add(tube('rim', rrect_path(-bx0, -by0, bx0, by0, 0.03, z, 4), r, 8, closed=True), 'chrome')
    xs = np.arange(-bx0 + 0.035, bx0 - 0.03, 0.045)
    ys = np.arange(-by0 + 0.035, by0 - 0.03, 0.045)
    for x in xs:
        for y in (-by0, by0):
            k.add(parts.rod('wire', (x, y, z0), (x, y, z1), 0.0022, 6), 'chrome')
    for y in ys:
        for x in (-bx0, bx0):
            k.add(parts.rod('wire', (x, y, z0), (x, y, z1), 0.0022, 6), 'chrome')
    for y in ys:
        k.add(parts.rod('floor', (-bx0, y, z0), (bx0, y, z0), 0.0022, 6), 'chrome')
    for x in np.arange(-bx0 + 0.06, bx0 - 0.05, 0.09):
        k.add(parts.rod('floor_x', (x, -by0, z0 - 0.0042), (x, by0, z0 - 0.0042), 0.0025, 6), 'chrome')
    # The valet rail overhead.
    k.add(tube('upright', bend([(-0.36, 0.0, zf), (-0.36, 0.0, 1.73), (0.36, 0.0, 1.73), (0.36, 0.0, zf)], 0.07, 10), 0.0125, 14), 'chrome')
    for sx in (-1, 1):
        k.add(tube('brace', [(sx * 0.36, -0.18, zf), (sx * 0.36, -0.01, 0.62)], 0.009, 10), 'chrome')
        k.add(tube('brace', [(sx * 0.36, 0.18, zf), (sx * 0.36, 0.01, 0.62)], 0.009, 10), 'chrome')
    # Hangers: two wire, one white plastic, hooked over the rail.
    for (x, yaw, kind) in ((-0.17, 6.0, 'wire'), (-0.03, -4.0, 'white'), (0.14, 9.0, 'plastic')):
        k.add(hanger(x, 1.73, yaw, kind), 'chrome' if kind == 'wire' else ('powder' if kind == 'white' else 'gloss'),
              0xffffff if kind == 'wire' else (0xe8e6e0 if kind == 'white' else 0xe6e4de))
    k.grime = [((0.0, 0.0, 1.73), 0.2, 0.3)]
    return [k.finish(40.0)]


def hanger(x, zr, yaw, kind):
    """A coat hanger over a rail along X at height zr (its body in the YZ plane, turned yaw degrees about Z)."""
    # Wire 3.4 mm across (a hair over a real hanger's 2.6: thinner shimmers to nothing under TSR at 2 m).
    rr = 0.0125 + (0.0021 if kind != 'plastic' else 0.004)
    hook = [(0.0, rr * math.cos(math.radians(a)), zr + rr * math.sin(math.radians(a))) for a in np.linspace(-40.0, 200.0, 14)]
    if kind == 'plastic':
        body = [(0.0, -0.014, zr - 0.035), (0.0, 0.0, zr - 0.07), (0.0, -0.06, zr - 0.10), (0.0, -0.18, zr - 0.17), (0.0, -0.205, zr - 0.19),
                (0.0, -0.200, zr - 0.205), (0.0, 0.200, zr - 0.205), (0.0, 0.205, zr - 0.19), (0.0, 0.18, zr - 0.17), (0.0, 0.06, zr - 0.10),
                (0.0, 0.004, zr - 0.07)]
        pts = hook + bend(body, 0.02, 6)
        r = 0.0038
    else:
        body = [(0.0, -0.016, zr - 0.04), (0.0, -0.004, zr - 0.07), (0.0, 0.004, zr - 0.078), (0.0, -0.004, zr - 0.086), (0.0, 0.0, zr - 0.09),
                (0.0, -0.20, zr - 0.215), (0.0, 0.20, zr - 0.215), (0.0, 0.006, zr - 0.088)]
        pts = hook + bend(body, 0.01, 5)
        r = 0.0017
    o = tube('hanger', pts, r, 8 if kind == 'plastic' else 6)
    o.data.transform(Matrix.Translation((x, 0.0, 0.0)) @ Matrix.Rotation(math.radians(yaw), 4, 'Z'))
    return o


# ================================================================== SM_WF_Counter

CTOP = 1.05


def draw_counter_sign(s, w, h):
    s.rect(0, 0, w, h, 0xf3f1ea, rough=0.35)
    s.rect(0, h - 70, w, 70, 0xc2185b, rough=0.35)
    s.mid('WASH & FOLD', w / 2, h - 35, 34, w - 40, 0xffffff, 'Black', tracking=1.08)
    s.mid('DROP-OFF SERVICE', w / 2, h - 92, 13, w - 60, 0x1e2a44, 'Black', tracking=1.3)
    pw = text_w(s.s, '$1.50', 'Black') * 48
    lw = text_w(s.s, '/ LB', 'Black') * 20
    x0 = (w - pw - lw - 8) / 2
    s.text('$1.50', x0, h - 167, 48, 0xc2185b, 'Black')
    s.text('/ LB', x0 + pw + 8, h - 167, 20, 0x1e2a44, 'Black')
    for i, ln in enumerate(('10 LB MINIMUM  ·  WASHED, DRIED & FOLDED', 'IN BY 10 AM, READY BY 5 PM', 'COMFORTERS & BLANKETS $15 EACH')):
        s.mid(ln, w / 2, h - 205 - i * 24, 11, w - 50, 0x2a2a2a, 'Bold')
    s.rect(30, 22, w - 60, 1.5, 0xc2185b)
    s.mid('ASK THE ATTENDANT', w / 2, 10, 9, w - 80, 0x1e2a44, 'Black', tracking=1.25)


def draw_keys(s, w, h):
    """A membrane keyboard, as the attendant sees it."""
    s.rect(0, 0, w, h, 0x2b2d31, rough=0.45)
    labels = [['CLK', 'VOID', 'RF', '%-', 'PO', 'RA'], ['7', '8', '9', 'DEPT1', 'DEPT2', 'SUBT'], ['4', '5', '6', 'DEPT3', 'DEPT4', 'CHK'],
              ['1', '2', '3', 'WASH', 'FOLD', 'CARD'], ['0', '00', '.', 'SOAP', 'X', 'CASH']]
    kw, kh = (w - 14) / 6, (h - 14) / 5
    for r, row in enumerate(labels):
        for c, lab in enumerate(row):
            x0 = 7 + c * kw
            y0 = h - 7 - (r + 1) * kh
            big = lab == 'CASH'
            col = 0xc4161c if big else (0x2f6fb5 if lab.startswith('DEPT') or lab in ('WASH', 'FOLD', 'SOAP') else (0xe6e2d6 if lab.isdigit() or lab in ('00', '.') else 0x8d9196))
            s.rect(x0 + 1.5, y0 + 1.5, kw - 3, kh - 3, col, rough=0.35)
            s.mid(lab, x0 + kw / 2, y0 + kh / 2, kh * 0.32, kw - 8, 0x141414 if col in (0xe6e2d6, 0x8d9196) else 0xffffff, 'Black')


def p_vfd(lines, ink=0x6effc8, ground=0x041410):
    def draw(s, w, h):
        s.rect(0, 0, w, h, ground, rough=0.06)
        n = len(lines)
        for i, (a, b) in enumerate(lines):
            y = h - (i + 0.5) * h / n
            s.mid(a, 4, y, h / n * 0.55, w * 0.55, ink, 'Bold', align='LEFT', rough=0.06)
            s.mid(b, w - 4, y, h / n * 0.55, w * 0.45, ink, 'Bold', align='RIGHT', rough=0.06)
    return draw


def p_ticket(num, name, lbs, price, ready):
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0xf2d24a, rough=0.75)
        s.circle(w / 2, h - 7, 3.0, 0x6a5a20, rough=0.8, n=20)
        s.mid('WASH & FOLD', w / 2, h - 17, 5.0, w - 8, 0x2a2a2a, 'Black')
        s.mid('No. ' + num, w / 2, h - 27, 7.0, w - 8, 0xb0161c, 'Black')
        s.rect(4, h - 33, w - 8, 0.5, 0x6a5a20)
        hand(s, name, 6, h - 46, 9.0, 0x1b2f7a, width=0.08, seed=len(name))
        for i, (a, b) in enumerate((('LBS', lbs), ('DUE', price), ('READY', ready))):
            s.text(a, 5, 22 - i * 8 + 4, 4.2, 0x3a3a3a, 'Bold')
            s.text(b, w - 5, 22 - i * 8 + 4, 4.6, 0x1b2f7a, 'Bold', align='RIGHT')
    return draw


def p_receipt(seed):
    def draw(s, w, h):
        rnd = random.Random(seed)
        s.rect(0, 0, w, h, 0xf4f2ec, rough=0.8)
        s.mid('WASH & FOLD', w / 2, h - 8, 4.6, w - 8, 0x3a3a3a, 'Black')
        s.mid('5TH ST · NORTHSIDE', w / 2, h - 14, 3.0, w - 8, 0x5a5a5a, 'Bold')
        s.text(f'10/0{rnd.randint(1, 5)}  {rnd.randint(7, 11)}:{rnd.randint(10, 59)} PM', 4, h - 21, 2.8, 0x5a5a5a, 'Bold')
        s.text(f'REG 1  #{rnd.randint(1000, 1099)}', w - 4, h - 21, 2.8, 0x5a5a5a, 'Bold', align='RIGHT')
        y = h - 29
        tot = 0.0
        for i in range(rnd.randint(3, 5)):
            item, p = rnd.choice((('WASH & FOLD', 18.60), ('SOAP', 1.25), ('DRY SHEETS', 0.75), ('COMFORTER', 15.0), ('WASH & FOLD', 21.30), ('BLEACH', 1.00)))
            s.text(item, 4, y, 3.2, 0x4a4a4a, 'Bold')
            s.text(f'{p:.2f}', w - 4, y, 3.2, 0x4a4a4a, 'Bold', align='RIGHT')
            tot += p
            y -= 5.2
        s.rect(4, y + 2, w - 8, 0.4, 0x6a6a6a)
        tax = round(tot * 0.0, 2)
        for lab, val, size in (('SUBTOTAL', tot, 3.0), ('TAX', tax, 3.0), ('TOTAL', tot + tax, 3.8)):
            y -= 5.4
            s.text(lab, 4, y, size, 0x3a3a3a, 'Black' if lab == 'TOTAL' else 'Bold')
            s.text(f'{val:.2f}', w - 4, y, size, 0x3a3a3a, 'Black' if lab == 'TOTAL' else 'Bold', align='RIGHT')
        paid = math.ceil(tot / 5.0) * 5.0
        for lab, val in (('CASH', paid), ('CHANGE', paid - tot)):
            y -= 5.0
            s.text(lab, 4, y, 3.0, 0x4a4a4a, 'Bold')
            s.text(f'{val:.2f}', w - 4, y, 3.0, 0x4a4a4a, 'Bold', align='RIGHT')
        for i in range(34):
            if rnd.random() < 0.7:
                s.rect(8 + i * (w - 16) / 34, 13, (w - 16) / 34 * rnd.choice((0.4, 0.6, 0.9)), 6, 0x3a3a3a)
        s.mid('THANK YOU · COME AGAIN', w / 2, 7, 2.8, w - 8, 0x5a5a5a, 'Bold')
    return draw


def draw_ring(s, w, h):
    s.rect(0, 0, w, h, 0xf3f1ea, rough=0.5)
    s.rect(3, 3, w - 6, h - 6, 0xc2185b, rough=0.5)
    s.rect(5, 5, w - 10, h - 10, 0xf3f1ea, rough=0.5)
    s.mid('PLEASE RING', w / 2, h * 0.64, h * 0.18, w - 16, 0x1e2a44, 'Black')
    s.mid('FOR ATTENDANT', w / 2, h * 0.34, h * 0.14, w - 16, 0xc2185b, 'Black')


def draw_jug(s, w, h):
    s.rect(0, 0, w, h, 0x1f5fbf, rough=0.4)
    s.mid('SUDZ', w / 2, h * 0.6, h * 0.3, w - 10, 0xffd23a, 'Black')
    s.mid('PROFESSIONAL · 2.5 GAL', w / 2, h * 0.25, h * 0.08, w - 10, 0xffffff, 'Bold')


FOLDS = [0xeceae2, 0x2c3e5c, 0x8a8c90, 0x9e2e2a, 0x3b5373, 0xd9c9a8, 0x2f6b4f, 0xc77d9a, 0x1c1c1e]


def bag(k, cx, cy, z0, size, seed, tag=None, facing=-1):
    """A wash-and-fold order: the folded laundry stacked (towels, shirts, jeans) in a clear bag (WF_Plastic) gathered
    into a knot on top, its ticket taped to the side facing `facing` (-1: -Y, the customer's side; +1: +Y)."""
    W, D, Hh = size
    rnd = random.Random(seed)
    z = z0 + 0.003
    while z < z0 + Hh - 0.05:
        th = rnd.uniform(0.028, 0.055)
        w, d = W - 0.035 - rnd.uniform(0.0, 0.035), D - 0.035 - rnd.uniform(0.0, 0.04)
        slab = parts.rbox('folded', (0.0, 0.0, 0.0), (w, d, th), min(th * 0.48, 0.022), 3)
        slab.data.transform(Matrix.Translation((cx + rnd.uniform(-0.01, 0.01), cy + rnd.uniform(-0.01, 0.01), z + th / 2))
                            @ Matrix.Rotation(rnd.uniform(-0.06, 0.06), 4, 'Z'))
        k.add(slab, 'cloth', rnd.choice(FOLDS))
        z += th
    top = z
    # The bag: a rounded box round the stack (an icosphere pushed out to a superellipsoid), wrinkled, gathered on top.
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=4, radius=1.0)
    hx, hy = W / 2 + 0.012, D / 2 + 0.012
    hz = (top - z0) / 2 + 0.012
    zc = z0 + hz - 0.004
    P = np.array([v.co[:] for v in bm.verts])
    wr = texmaps.value_noise(P * 0.5 + seed, 0.18, seed=seed, octaves=2) - 0.5
    for v, w_ in zip(bm.verts, wr):
        d = v.co.normalized()
        pn = (abs(d.x) ** 5 + abs(d.y) ** 5 + abs(d.z) ** 5) ** 0.2
        q = d / pn
        x, y, zz = q.x * hx, q.y * hy, q.z * hz
        g = float(texmaps.smoothstep(0.55, 1.0, d.z))
        x *= 1.0 - 0.86 * g
        y *= 1.0 - 0.86 * g
        zz += g * 0.05
        rr = 1.0 + 0.04 * w_ + g * 0.12 * math.sin(math.atan2(d.y, d.x) * 9.0)
        v.co = Vector((cx + x * rr, cy + y * rr, max(z0 + 0.001, zc + zz)))
    b = core.mesh_object('bag', bm)
    b.data.shade_smooth()
    k.special(b, 'WF_Plastic')
    neck = zc + hz + 0.05
    knot = blob('knot', (cx, cy, neck + 0.008), (0.045, 0.04, 0.035), seed=seed + 3, detail=0.25, subdiv=2)
    k.special(knot, 'WF_Plastic')
    for sgn in (-1, 1):
        tail = blob('tail', (cx + sgn * 0.03, cy + 0.006, neck + 0.004), (0.05, 0.016, 0.025), seed=seed + 5 + sgn, detail=0.3, subdiv=2)
        k.special(tail, 'WF_Plastic')
    if tag:
        yface = cy + facing * (hy * 1.03 + 0.001)
        u = (-facing, 0.0, 0.0)
        k.decal(tag, (cx + 0.03 * -facing, yface, zc + hz * 0.15), u=u, v=(0.0, 0.0, 1.0), lift=0.0)
        k.decal('tape', (cx + 0.03 * -facing, yface + facing * 0.0006, zc + hz * 0.15 + 0.04), u=u, v=(0.0, 0.0, 1.0), lift=0.0, w=0.04)
    return neck


def counter():
    k = Kit('wfo', 'SM_WF_Counter', dust=0.1)
    k.panel('sign', 0.46, 0.30, draw_counter_sign)
    k.panel('keys', 0.30, 0.16, draw_keys, density=1.4)
    k.panel('clerk', 0.10, 0.03, p_vfd([('SUBTL', '18.60'), ('CASH', '20.00')]), glow=3.0, density=2.0)
    k.panel('cust', 0.12, 0.035, p_vfd([('TOTAL', '18.60')]), glow=3.0, density=2.0)
    for i, (num, nm, lbs, pr, rd) in enumerate((('1047', 'Perez', '12.4', '$18.60', 'TUE 5P'), ('1052', 'Okafor', '14.2', '$21.30', 'TUE 5P'),
                                               ('1049', 'Dee', '9.0', '$15.00', 'TUE'))):
        k.panel(f't{i}', 0.055, 0.085, p_ticket(num, nm, lbs, pr, rd), density=2.2)
    k.panel('r0', 0.058, 0.10, p_receipt(3), density=1.6)
    k.panel('r1', 0.058, 0.10, p_receipt(8), density=1.6)
    k.panel('ring', 0.10, 0.06, draw_ring, density=1.6)
    k.panel('scale', 0.07, 0.022, p_vfd([('LB', '0.00')], 0xff5a3a, 0x140404), glow=3.0, density=2.0)
    k.panel('jug', 0.10, 0.08, draw_jug)
    k.panel('tape', 0.045, 0.016, draw_tape, density=1.2)
    k.panel('regbrand', 0.07, 0.014, p_label('CASHMATE ER-220', 0x2d2f33, 0xb9bcc0, 'Bold'), density=2.0)
    k.atlas.render()
    rnd = random.Random(1047)
    teal, white, top_col = 0x4c7d77, 0xe8e6df, 0xdcd8cd
    # The carcass: a teal laminate front on a black vinyl base, a stainless bumper strip, the top with rounded edges;
    # behind, white melamine shelves and a divider.
    k.add(bx('front', -0.99, 0.99, -0.30, -0.28, 0.10, CTOP - 0.038, 0.003), 'solid', teal)
    k.add(bx('base', -0.99, 0.99, -0.28, -0.26, 0.0, 0.10, 0.002), 'rubber', 0x1e1e1e)
    k.add(bx('bumper', -0.99, 0.99, -0.307, -0.299, 0.29, 0.32, 0.004, 3), 'steel')
    for sx in (-1, 1):
        k.add(bx('end', sx * 0.99 - 0.01, sx * 0.99 + 0.01, -0.30, 0.31, 0.0, CTOP - 0.038, 0.003), 'solid', teal)
    k.add(bx('top', -1.02, 1.02, -0.345, 0.32, CTOP - 0.038, CTOP, 0.012, 4), 'laminate', top_col)
    k.add(bx('skin', -0.98, 0.98, -0.28, -0.272, 0.12, CTOP - 0.038), 'solid', white)
    k.add(bx('shelf_lo', -0.98, 0.98, -0.272, 0.30, 0.10, 0.12, 0.002), 'solid', white)
    k.add(bx('shelf_mid', -0.98, 0.98, -0.272, 0.30, 0.52, 0.54, 0.002), 'solid', white)
    k.add(bx('divider', -0.01, 0.01, -0.272, 0.30, 0.54, CTOP - 0.038), 'laminate', white)
    k.add(bx('divider_lo', -0.01, 0.01, -0.272, 0.30, 0.12, 0.52), 'solid', white)
    k.add(bx('rail', -0.98, 0.98, 0.28, 0.30, 0.95, CTOP - 0.038, 0.002), 'solid', white)
    k.add(bx('sign_plate', -0.685, -0.215, -0.304, -0.30, 0.505, 0.815, 0.002), 'plastic', 0xf1efe8)
    k.decal('sign', (-0.45, -0.304, 0.66))
    # The register: drawer base, the sloped keyboard toward the attendant (+Y), the displays both ways, the printer.
    rx = 0.45
    k.add(bx('reg_base', rx - 0.20, rx + 0.20, -0.20, 0.22, CTOP, CTOP + 0.10, 0.008, 3), 'plastic', 0x2d2f33)
    k.add(bx('reg_drawer', rx - 0.18, rx + 0.18, 0.218, 0.228, CTOP + 0.006, CTOP + 0.092, 0.004), 'plastic', 0x3a3c40)
    k.add(disc_y('reg_lock', rx + 0.14, 0.228, CTOP + 0.07, 0.008, -0.004, 16), 'chrome')
    k.decal('regbrand', (rx, -0.2002, CTOP + 0.07), lift=0.0003)
    body = core.prism_x('reg_body', [(-0.20, CTOP + 0.10), (0.22, CTOP + 0.10), (0.22, CTOP + 0.12), (0.02, CTOP + 0.165), (-0.08, CTOP + 0.19),
                                     (-0.20, CTOP + 0.19)], rx - 0.19, rx + 0.19)
    k.add(body, 'plastic', 0x34363a)
    va = Vector((0.0, 0.02 - 0.22, 0.165 - 0.12))
    k.decal('keys', (rx, 0.12, CTOP + 0.1425), u=(-1, 0, 0), v=tuple(va.normalized()), lift=0.0005)
    k.add(bx('pod', rx - 0.15, rx - 0.03, -0.075, -0.02, CTOP + 0.168, CTOP + 0.235, 0.006, 3), 'plastic', 0x2b2d31)
    k.decal('clerk', (rx - 0.09, -0.02, CTOP + 0.21), u=(-1, 0, 0), v=(0, 0, 1), lift=0.0004)
    k.add(parts.rod('pole', (rx + 0.13, -0.15, CTOP + 0.19), (rx + 0.13, -0.15, CTOP + 0.29), 0.011, 16), 'plastic', 0x2b2d31)
    k.add(bx('cust_head', rx + 0.06, rx + 0.20, -0.172, -0.128, CTOP + 0.28, CTOP + 0.34, 0.008, 3), 'plastic', 0x2b2d31)
    k.decal('cust', (rx + 0.13, -0.172, CTOP + 0.31), lift=0.0004)
    k.add(bx('printer', rx + 0.03, rx + 0.17, -0.19, -0.07, CTOP + 0.19, CTOP + 0.215, 0.008, 3), 'plastic', 0x45474b)
    k.decal('r0', (rx + 0.10, -0.115, CTOP + 0.2154), u=(1, 0, 0), v=(0, 1, 0), w=0.058, h=0.05, lift=0.0004, nu=1, nv=3,
            disp=lambda s, t: 0.012 * t * t)
    # The receipt spike and its slips.
    sx, sy = 0.86, 0.10
    k.add(lathe_axis('spike_base', [(0.0, 0.0), (0.040, 0.0), (0.042, 0.004), (0.040, 0.012), (0.014, 0.016), (0.0, 0.016)],
                     Matrix.Translation((sx, sy, CTOP)), 40), 'powder', 0x161616)
    k.add(lathe_axis('spike', [(0.0, 0.0), (0.0018, 0.0), (0.0015, 0.13), (0.0, 0.155)], Matrix.Translation((sx, sy, CTOP + 0.016)), 12), 'steel')
    for j in range(9):
        M = (Matrix.Translation((sx, sy, CTOP + 0.022 + j * 0.006)) @ Matrix.Rotation(rnd.uniform(-0.8, 0.8), 4, 'Z')
             @ Matrix.Rotation(rnd.uniform(-0.06, 0.06), 4, 'X') @ Matrix.Translation((0.0, -0.036, 0.0)))
        k.decal('r1' if j % 2 else 'r0', (0.0, 0.0, 0.0), u=(1, 0, 0), v=(0, 1, 0), lift=0.0, nu=2, nv=3,
                disp=lambda s, t, a=rnd.uniform(0.004, 0.012): a * (t - 0.6) ** 2 * (1 if t < 0.6 else 0.4), matrix=M)
    # The call bell and its tent card.
    bxp, byp = 0.10, -0.25
    k.add(lathe_axis('bell_base', [(0.0, 0.0), (0.046, 0.0), (0.047, 0.004), (0.045, 0.014), (0.030, 0.018), (0.0, 0.018)],
                     Matrix.Translation((bxp, byp, CTOP)), 48), 'powder', 0x121212)
    dome = [(0.0, 0.0185)] + [(0.042 * math.sin(math.radians(a)), 0.0185 + 0.034 * math.cos(math.radians(a))) for a in range(90, -1, -10)]
    k.add(lathe_axis('bell', dome[:1] + dome[1:], Matrix.Translation((bxp, byp, CTOP)), 48), 'chrome')
    k.add(parts.rod('plunger', (bxp, byp, CTOP + 0.052), (bxp, byp, CTOP + 0.064), 0.003, 12), 'chrome')
    k.add(parts.rod('plunger_cap', (bxp, byp, CTOP + 0.064), (bxp, byp, CTOP + 0.07), 0.008, 20), 'chrome')
    a = math.radians(14.0)
    ty, tz = -0.255, CTOP + 0.06 * math.cos(a)
    k.decal('ring', (0.24, ty - 0.03 * math.sin(a), CTOP + 0.03 * math.cos(a)), u=(1, 0, 0), v=(0, math.sin(a), math.cos(a)), lift=0.0003)
    k.add(quad('card_back', [(0.19, ty, tz), (0.29, ty, tz), (0.29, ty + 0.06 * math.sin(a), CTOP), (0.19, ty + 0.06 * math.sin(a), CTOP)]), 'paper', 0xf3f1ea)
    k.add(quad('card_front', [(0.19, ty - 0.06 * math.sin(a), CTOP), (0.29, ty - 0.06 * math.sin(a), CTOP), (0.29, ty, tz), (0.19, ty, tz)]), 'paper', 0xf3f1ea)
    # The platform scale.
    k.add(bx('scale_base', -0.35, -0.05, -0.13, 0.17, CTOP, CTOP + 0.05, 0.01, 3), 'plastic', 0x2b2d30)
    k.add(bx('scale_plate', -0.34, -0.06, -0.10, 0.16, CTOP + 0.05, CTOP + 0.058, 0.004), 'steel')
    k.add(bx('scale_pod', -0.26, -0.14, -0.205, -0.13, CTOP, CTOP + 0.042, 0.008, 3), 'plastic', 0x2b2d30)
    k.decal('scale', (-0.20, -0.205, CTOP + 0.022), lift=0.0004)
    # Three bagged orders with their tickets.
    for i, (cx, cy, zf, size, facing) in enumerate(((-0.84, -0.03, CTOP, (0.30, 0.36, 0.22), -1), (-0.52, 0.0, CTOP, (0.28, 0.34, 0.26), -1),
                                                    (-0.52, 0.05, 0.12, (0.32, 0.36, 0.24), 1))):
        bag(k, cx, cy, zf, size, seed=40 + i * 7, tag=f't{i}', facing=facing)
    # Behind: a roll of bags, a spray bottle, a bulk jug, a carton of hangers.
    k.add(parts.rod('roll', (0.18, 0.12, 0.61), (0.62, 0.12, 0.61), 0.07, 32), 'bag', 0xd6dee8)
    k.add(parts.rod('roll_core', (0.175, 0.12, 0.61), (0.625, 0.12, 0.61), 0.021, 20), 'paper', 0xa88a5a)
    k.add(lathe_axis('bottle', [(0.0, 0.0), (0.038, 0.0), (0.041, 0.01), (0.041, 0.17), (0.034, 0.20), (0.013, 0.22), (0.013, 0.24), (0.0, 0.24)],
                     Matrix.Translation((-0.40, 0.16, 0.54)), 32), 'gloss', 0x3a8fd0)
    k.add(bx('trigger_head', -0.415, -0.385, 0.10, 0.175, 0.775, 0.81, 0.008, 3), 'plastic', 0xe9e7e0)
    k.add(bx('trigger', -0.408, -0.392, 0.11, 0.125, 0.745, 0.78, 0.004), 'plastic', 0xe9e7e0)
    k.add(bx('jug', -0.79, -0.63, 0.06, 0.17, 0.54, 0.82, 0.02, 3), 'plastic', 0xe8eef2)
    k.add(parts.rod('jug_cap', (-0.67, 0.115, 0.82), (-0.67, 0.115, 0.845), 0.022, 24), 'plastic', 0x1f6fd0)
    k.add(tube('jug_handle', core.catmull_rom([(-0.78, 0.115, 0.79), (-0.80, 0.115, 0.80), (-0.79, 0.115, 0.83), (-0.74, 0.115, 0.83)], 4), 0.012, 10),
          'plastic', 0xe8eef2)
    k.decal('jug', (-0.71, 0.1704, 0.68), u=(-1, 0, 0), v=(0, 0, 1))
    k.add(bx('carton', 0.30, 0.72, -0.05, 0.27, 0.12, 0.36, 0.004), 'paper', 0xa8875a)
    k.grime = [((bxp, byp, CTOP), 0.12, 0.5), ((0.0, -0.33, CTOP), 0.5, 0.4), ((rx, 0.22, CTOP + 0.05), 0.2, 0.5)]
    return [k.finish()]


# ================================================================== SM_WF_BackDoor (frame, leaf; joined in assemble)

def draw_private(s, w, h):
    """Two-ply engraved plastic: red skin, white core where the letters are cut."""
    s.rect(0, 0, w, h, 0xa3221c, rough=0.35)
    s.rect(3, 3, w - 6, h - 6, 0xf4f1ea, rough=0.4)
    s.rect(5, 5, w - 10, h - 10, 0xa3221c, rough=0.35)
    s.mid('PRIVATE', w / 2, h * 0.62, h * 0.36, w - 30, 0xf4f1ea, 'Black', tracking=1.25, rough=0.4)
    s.mid('STAFF ONLY', w / 2, h * 0.24, h * 0.18, w - 60, 0xf4f1ea, 'Black', tracking=1.5, rough=0.4)


def draw_card(s, w, h):
    """Dee's index card, in blue ballpoint pressed hard: Tue Thu Sat / 9 PM."""
    s.rect(0, 0, w, h, 0xf7f4ea, rough=0.85)
    s.rect(0, h - 12, w, 0.8, 0xd8a0a0, rough=0.85)
    for i in range(5):
        s.rect(0, 8 + i * 11, w, 0.45, 0xa9c4d8, rough=0.85)
    ink = 0x1c2f86
    hand(s, 'Tue  Thu  Sat', 12, h - 34, 15.0, ink, width=0.075, seed=3)
    x = hand(s, '9 PM', 34, 14, 20.0, ink, width=0.08, seed=9)
    s.stroke([(30, 10), (60, 8.6), (x - 6, 9.6)], 1.5, ink)
    hand(s, '- D.', w - 30, 6, 9.0, ink, width=0.09, seed=4)


def door_frame():
    k = Kit('wfbf', 'SM_WF_BackDoor_Frame', dust=0.3, low_grime=0.18)
    paint = 0x3f3c38
    # Hollow-metal frame: jambs full height (their faces 50 mm), the head between them, the stops behind the faces.
    for sx in (-1, 1):
        x0, x1 = sorted((sx * 0.46, sx * 0.51))
        k.add(bx('jamb', x0, x1, -0.108, 0.108, 0.0, 2.17, 0.0025, 2), 'paint', paint)
        s0, s1 = sorted((sx * 0.444, sx * 0.4605))
        k.add(bx('stop', s0, s1, -0.060, -0.010, 0.0, 2.12, 0.0015, 2), 'paint', paint)
    k.add(bx('head', -0.46, 0.46, -0.108, 0.108, 2.12, 2.17, 0.0025, 2), 'paint', paint)
    k.add(bx('head_stop', -0.444, 0.444, -0.060, -0.010, 2.104, 2.1205, 0.0015, 2), 'paint', paint)
    # The strike on the latch jamb, the hinges' frame leaves on the other.
    k.add(bx('strike', 0.4585, 0.4605, -0.008, 0.032, 0.96, 1.08, 0.0005), 'steel')
    k.add(bx('strike_hole', 0.4580, 0.4590, 0.004, 0.022, 1.0, 1.03), 'rubber', 0x080808)
    for z in (0.25, 1.06, 1.87):
        k.add(bx('hinge_leaf', -0.4605, -0.4595, 0.012, 0.040, z - 0.055, z + 0.055, 0.0004), 'steel')
    for z in (0.40, 1.10, 1.80):
        k.add(parts.rod('silencer', (0.452, -0.0095, z), (0.452, -0.0085, z), 0.004, 10), 'rubber', 0x2a2a2a)
    k.grime = [((0.46, -0.11, 1.05), 0.15, 0.7), ((-0.46, -0.11, 1.05), 0.12, 0.4)]
    return [k.finish(40.0)]


def door_leaf():
    """Built in the door's frame, then moved so its origin is the hinge axis (LEAF_PIVOT)."""
    k = Kit('wfbl', 'SM_WF_BackDoor_Leaf', dust=0.25, low_grime=0.25)
    k.panel('private', 0.296, 0.096, draw_private, density=1.3)
    k.panel('card', 0.127, 0.076, draw_card, density=2.2)
    k.panel('tape', 0.045, 0.016, draw_tape, density=1.2)
    k.panel('push', 0.10, 0.018, p_label('PUSH', 0x18191a, 0xd6d6d0), density=1.5)
    k.panel('plate', 0.10, 0.02, p_plate(['FIRE DOOR  ·  20 MIN  ·  KEEP CLOSED'], 0xbdbab1, 0x2a2a2a), density=2.0)
    k.atlas.render()
    paint = 0x52605d
    yf, yb = -0.0095, 0.0355
    k.add(bx('leaf', -0.455, 0.455, yf, yb, 0.012, 2.115, 0.003, 2), 'paint', paint)
    # Front: kick plate with its screws, the touch bar, the deadbolt, the sign, Dee's card.
    k.add(bx('kick', -0.43, 0.43, yf - 0.0013, yf, 0.02, 0.26, 0.0004), 'steel')
    for x in (-0.415, -0.14, 0.14, 0.415):
        for z in (0.035, 0.245):
            k.add(disc_y('screw', x, yf - 0.0013, z, 0.003, 0.0012, 10), 'chrome')
    k.add(bx('bar_rail', -0.40, 0.31, -0.068, yf, 0.988, 1.042, 0.010, 3), 'alu')
    k.add(bx('bar_case', 0.30, 0.425, -0.076, yf, 0.965, 1.065, 0.014, 3), 'alu')
    k.add(bx('bar_cap', -0.42, -0.392, -0.072, yf, 0.982, 1.048, 0.008, 3), 'plastic', 0x18191a)
    k.add(bx('touchpad', -0.26, 0.26, -0.080, -0.066, 0.995, 1.035, 0.007, 3), 'plastic', 0x1a1b1c)
    k.decal('push', (0.0, -0.080, 1.015), lift=0.0004)
    k.add(disc_y('db_rose', 0.385, yf, 1.20, 0.028, 0.008, 40), 'chrome')
    k.add(disc_y('db_cyl', 0.385, yf - 0.008, 1.20, 0.016, 0.005, 32), 'chrome')
    k.add(bx('db_key', 0.3845, 0.3855, yf - 0.0135, yf - 0.012, 1.191, 1.209), 'rubber', 0x060606)
    k.add(bx('sign', -0.15, 0.15, yf - 0.003, yf, 1.50, 1.60, 0.0015), 'plastic', 0xa3221c)
    k.decal('private', (0.0, yf - 0.003, 1.55), lift=0.0003)
    for x in (-0.138, 0.138):
        k.add(disc_y('sign_screw', x, yf - 0.003, 1.55, 0.0035, 0.0015, 12), 'chrome')
    a = math.radians(-3.0)
    cu, cv = (math.cos(a), 0.0, math.sin(a)), (-math.sin(a), 0.0, math.cos(a))
    cc = Vector((0.02, yf, 1.385))
    k.decal('card', tuple(cc), u=cu, v=cv, lift=0.0005, nu=4, nv=2, disp=lambda s, t: 0.0025 * (1 - t) ** 2 * abs(2 * s - 1) ** 1.5)
    for side, ang in ((-1, 32.0), (1, -28.0)):
        p = cc + Vector(cu) * (side * 0.058) + Vector(cv) * 0.034
        tape(k, tuple(p), ang - 3.0, lift=0.0009)
    k.decal('plate', (-0.38, yf, 2.06), lift=0.0003)
    # Back: the pull, the closer, the hinges' knuckles.
    k.add(tube('pull', bend([(0.39, yb, 0.90), (0.39, yb + 0.045, 0.92), (0.39, yb + 0.045, 1.10), (0.39, yb, 1.12)], 0.02, 6), 0.0095, 14), 'steel')
    for z in (0.90, 1.12):
        k.add(parts.rod('pull_rose', (0.39, yb, z), (0.39, yb + 0.006, z), 0.018, 20), 'steel')
    k.add(bx('closer', -0.40, -0.13, yb, yb + 0.058, 1.975, 2.045, 0.012, 3), 'powder', 0x8c8f92)
    k.add(bx('closer_arm', -0.20, 0.08, yb + 0.058, yb + 0.072, 2.03, 2.048, 0.004), 'powder', 0x2a2b2d)
    k.add(parts.rod('closer_pivot', (-0.17, yb + 0.03, 2.048), (-0.17, yb + 0.03, 2.06), 0.012, 16), 'powder', 0x2a2b2d)
    px, py = LEAF_PIVOT[0], LEAF_PIVOT[1]
    for z in (0.25, 1.06, 1.87):
        k.add(parts.rod('knuckle', (px, py, z - 0.055), (px, py, z + 0.055), 0.0062, 16), 'steel')
        k.add(parts.rod('pin_cap', (px, py, z + 0.055), (px, py, z + 0.060), 0.0055, 12), 'steel')
        k.add(bx('hinge_leaf', -0.455, px + 0.004, yb, yb + 0.0015, z - 0.055, z + 0.055, 0.0004), 'steel')
    k.grime = [((0.36 - px, -0.07 - py, 1.0), 0.13, 1.0), ((-0.35 - px, -0.07 - py, 1.0), 0.10, 0.7), ((0.385 - px, yf - py, 1.2), 0.06, 0.7),
               ((0.39 - px, yb + 0.04 - py, 1.0), 0.12, 0.9), ((0.43 - px, -0.01 - py, 1.0), 0.25, 0.6)]
    for o in k.objs:
        o.data.transform(Matrix.Translation((-px, -py, 0.0)))
    return [k.finish(40.0)]


# ================================================================== SM_WF_WallDressing

BOARD = (-0.765, 1.13, 0.135, 1.77)    # the cork board's frame (x0, z0, x1, z1)
RIGHT_X = 0.585


def photocopy(s, w, h, seed, amount=1.0):
    """Toner specks and the copier's grey edge: a flyer run off at the library."""
    rnd = random.Random(seed)
    if seed % 2 == 0:
        # folded in half to carry it here: a crease across the middle, shadowed on one side, lit on the other
        s.rect(0, h / 2 - 0.5, w, 0.5, 0x6a6a60, rough=0.85, alpha=0.22)
        s.rect(0, h / 2, w, 0.7, 0xffffff, rough=0.85, alpha=0.25)
    if seed % 3 == 1:
        s.rect(0, 0, w, h, 0xc9b27a, rough=0.85, alpha=0.10)   # weeks on the board: yellowed
    s.rect(0, 0, w, 3.0, 0x6a6a66, rough=0.8, alpha=0.18 * amount)
    s.rect(w - 2.5, 0, 2.5, h, 0x6a6a66, rough=0.8, alpha=0.12 * amount)
    for _ in range(int(70 * amount)):
        s.circle(rnd.uniform(2, w - 2), rnd.uniform(2, h - 2), rnd.uniform(0.12, 0.45), 0x3a3a38, rough=0.8, n=8, alpha=rnd.uniform(0.3, 0.8))


def flyer_text(s, w, h, title, lines, title_col=0x141414, ground=0xf4f2ea, title_size=None, face='Black', foot=0.0, seed=1):
    """A title at the top, the lines spread evenly down to foot (millimeters from the bottom)."""
    s.rect(0, 0, w, h, ground, rough=0.8)
    ts = title_size or h * 0.11
    s.mid(title, w / 2, h - ts * 0.95, ts, w - 12, title_col, face)
    top, bot = h - ts * 1.75, foot + 6.0
    total = sum(size * 1.25 for _, size, _ in lines)
    gap = max(0.0, (top - bot - total) / (len(lines) + 1))
    y = top - gap
    for ln, size, col in lines:
        y -= size * 0.62
        s.mid(ln, w / 2, y, size, w - 14, col, 'Bold')
        y -= size * 0.63 + gap
    photocopy(s, w, h, seed)
    return y


def draw_rent(s, w, h):
    flyer_text(s, w, h, 'ROOM FOR RENT', [('$650 / MONTH', 15, 0xb0161c), ('UTILITIES INCLUDED', 8, 0x2a2a2a), ('QUIET BUILDING · NO SMOKING', 7, 0x2a2a2a),
                                           ('FURNISHED · SHARED BATH', 7, 0x2a2a2a), ('LAUNDRY DOWNSTAIRS!', 7, 0x2a2a2a), ('5 MIN TO THE 14 BUS', 7, 0x2a2a2a)],
               foot=30, seed=2)
    s.rect(10, 26, w - 20, 0.8, 0x6a6a6a)
    s.mid('CALL OR TEXT 555-0148', w / 2, 16, 8, w - 16, 0x141414, 'Black')


def draw_guitar(s, w, h):
    tab_h = h * 0.25
    s.rect(0, 0, w, h, 0xfff3a8, rough=0.8)
    s.mid('GUITAR LESSONS', w / 2, h - 16, 12, w - 12, 0x141414, 'Black')
    cx, cy = w * 0.5, h * 0.6
    s.circle(cx - 6, cy - 10, 17, 0x8a4a1c, n=40)
    s.circle(cx + 9, cy + 6, 13, 0x8a4a1c, n=40)
    s.circle(cx - 2, cy - 3, 5, 0x2a1a0c, n=24)
    s.poly([(cx + 14, cy + 14), (cx + 46, cy + 46), (cx + 43, cy + 49), (cx + 11, cy + 17)], 0x3a2a1a)
    s.mid('ALL AGES · ALL LEVELS', w / 2, tab_h + 26, 6.5, w - 14, 0x2a2a2a, 'Bold')
    s.mid('$25 / HALF HOUR', w / 2, tab_h + 13, 7.5, w - 14, 0xb0161c, 'Black')
    for i in range(8):
        x0 = i * w / 8
        s.rect(x0 + w / 8 - 0.4, 0, 0.8, tab_h, 0xb8ae70)
        s.text('GUITAR  555-0193', x0 + w / 16 + 2.2, 3, 5.0, 0x141414, 'Bold', rot=math.pi / 2)
    s.rect(0, tab_h - 0.4, w, 0.8, 0xb8ae70)
    photocopy(s, w, h, 7, 0.6)


def draw_cat(s, w, h):
    s.rect(0, 0, w, h, 0xf4f2ea, rough=0.8)
    s.mid('LOST CAT', w / 2, h - 20, 22, w - 12, 0xc4161c, 'Black')
    px0, py0, pw, ph = 18, h * 0.38, w - 36, h * 0.4
    s.rect(px0, py0, pw, ph, 0x6b6e70)
    s.rect(px0 + 2, py0 + 2, pw - 4, ph - 4, 0x9a9690)
    cx, cy = w / 2, py0 + ph * 0.42
    s.circle(cx, cy, 26, 0xd1883a, n=48)
    s.poly([(cx - 24, cy + 10), (cx - 18, cy + 38), (cx - 6, cy + 22)], 0xd1883a)
    s.poly([(cx + 24, cy + 10), (cx + 18, cy + 38), (cx + 6, cy + 22)], 0xd1883a)
    s.circle(cx - 9, cy + 4, 4, 0x2b3a1a, n=16)
    s.circle(cx + 9, cy + 4, 4, 0x2b3a1a, n=16)
    s.poly([(cx - 3, cy - 6), (cx + 3, cy - 6), (cx, cy - 10)], 0xa0503a)
    for i, ln in enumerate(('ANSWERS TO "BISCUIT"', 'ORANGE TABBY · SHY · NO COLLAR', 'LAST SEEN 5TH & MARKET')):
        s.mid(ln, w / 2, py0 - 12 - i * 11, 6.5, w - 14, 0x2a2a2a, 'Bold')
    s.mid('REWARD  ·  555-0127', w / 2, 12, 9, w - 14, 0x141414, 'Black')
    photocopy(s, w, h, 6)


def draw_clean(s, w, h):
    flyer_text(s, w, h, 'HOUSE CLEANING', [('RELIABLE · REFERENCES', 7.5, 0x1f4f8f), ('WEEKLY OR EVERY 2 WEEKS', 6.5, 0x2a2a2a), ('APARTMENTS · OFFICES', 6.5, 0x2a2a2a),
                                            ('MOVE-OUTS · DEEP CLEANS', 6.5, 0x2a2a2a), ('ASK FOR MARISOL', 8, 0x2a2a2a)], title_col=0x1f4f8f, ground=0xdcecf4, foot=22, seed=3)
    s.mid('555-0182', w / 2, 14, 11, w - 16, 0x141414, 'Black')


def draw_keys_found(s, w, h):
    """An index card in the attendant's black marker."""
    s.rect(0, 0, w, h, 0xf7f4ea, rough=0.85)
    s.rect(0, h - 12, w, 0.8, 0xd8a0a0, rough=0.85)
    for i in range(5):
        s.rect(0, 8 + i * 11, w, 0.45, 0xa9c4d8, rough=0.85)
    ink = 0x161616
    x = hand(s, 'Found keys', 10, h - 30, 15.0, ink, width=0.1, seed=21)
    s.stroke([(10, h - 33.5), (x * 0.6, h - 34.5), (x - 4, h - 33.8)], 1.6, ink)
    hand(s, 'on the folding table', 10, h - 49, 10.5, ink, width=0.1, seed=22)
    hand(s, 'ask attendant', 26, 9, 10.5, ink, width=0.1, seed=23)


def draw_sale(s, w, h):
    flyer_text(s, w, h, 'MOVING SALE', [('SATURDAY 8 AM - 2 PM', 9, 0xb0161c), ('FURNITURE · TOOLS · RECORDS', 6.5, 0x2a2a2a), ('EVERYTHING MUST GO', 7, 0x2a2a2a),
                                         ('212 MARKET ST, APT 3', 7, 0x2a2a2a)], ground=0xffd9a8, title_size=16, seed=4)


def draw_tax(s, w, h):
    flyer_text(s, w, h, 'TAX PREP', [('FROM $49', 14, 0x2f7a3a), ('BILINGUAL · FAST', 7, 0x2a2a2a), ('E-FILE INCLUDED', 7, 0x2a2a2a),
                                      ('WALK-INS WELCOME', 7, 0x2a2a2a), ('FIFTH ST. SERVICES', 7, 0x2a2a2a)], title_col=0x2f7a3a, ground=0xe8f2e0, title_size=18,
               foot=20, seed=5)
    s.mid('555-0136', w / 2, 12, 9, w - 16, 0x141414, 'Black')


def p_bcard(name, line, phone, ground, ink):
    def draw(s, w, h):
        s.rect(0, 0, w, h, ground, rough=0.6)
        s.mid(name, w / 2, h * 0.66, h * 0.17, w - 10, ink, 'Black')
        s.mid(line, w / 2, h * 0.42, h * 0.1, w - 10, 0x3a3a3a, 'Bold')
        s.mid(phone, w / 2, h * 0.18, h * 0.12, w - 10, ink, 'Bold')
    return draw


def draw_open24(s, w, h):
    s.rect(0, 0, w, h, 0xc4161c, rough=0.35)
    s.rect(5, 5, w - 10, h - 10, 0xf6f3ea, rough=0.35)
    s.mid('OPEN 24 HOURS', w / 2, h * 0.6, h * 0.42, w - 40, 0xc4161c, 'Black', tracking=1.08)
    s.mid('7 DAYS A WEEK  ·  365 DAYS A YEAR', w / 2, h * 0.2, h * 0.12, w - 80, 0x1e2a44, 'Black', tracking=1.15)


def draw_attendant(s, w, h):
    s.rect(0, 0, w, h, 0x1e4f8f, rough=0.35)
    s.rect(4, 4, w - 8, h - 8, 0x24599e, rough=0.35)
    s.mid('ATTENDANT ON DUTY', w / 2, h * 0.6, h * 0.3, w - 24, 0xffffff, 'Black', tracking=1.1)
    s.mid('7 AM - 9 PM DAILY', w / 2, h * 0.22, h * 0.15, w - 60, 0xffd23a, 'Black', tracking=1.2)


def draw_nodye(s, w, h):
    s.rect(0, 0, w, h, 0xf6f3ea, rough=0.35)
    s.rect(6, 6, w - 12, h - 12, 0xc4161c, rough=0.35)
    s.rect(10, 10, w - 20, h - 20, 0xf6f3ea, rough=0.35)
    s.mid('NO DYEING', w / 2, h * 0.68, h * 0.2, w - 36, 0xc4161c, 'Black', tracking=1.05)
    s.mid('IN THE MACHINES', w / 2, h * 0.47, h * 0.09, w - 50, 0x141414, 'Black', tracking=1.2)
    s.rect(30, h * 0.36, w - 60, 1.2, 0x141414)
    s.mid('VIOLATORS WILL BE CHARGED', w / 2, h * 0.26, h * 0.065, w - 50, 0x141414, 'Bold')
    s.mid('FOR CLEANING  ·  $50 MIN', w / 2, h * 0.16, h * 0.065, w - 50, 0x141414, 'Bold')
    s.mid('- THE MANAGEMENT', w / 2, h * 0.07, h * 0.045, w - 80, 0x5a5a5a, 'Bold')


def draw_notresp(s, w, h):
    s.rect(0, 0, w, h, 0xf2c230, rough=0.4)
    s.mid('NOT RESPONSIBLE', w / 2, h * 0.7, h * 0.22, w - 20, 0x141414, 'Black', tracking=1.1)
    s.mid('FOR LOST OR STOLEN ITEMS', w / 2, h * 0.42, h * 0.13, w - 30, 0x141414, 'Black')
    s.mid('PLEASE DO NOT LEAVE MACHINES UNATTENDED', w / 2, h * 0.16, h * 0.09, w - 30, 0x2a2a2a, 'Bold')


def wall_dressing():
    k = Kit('wfg', 'SM_WF_WallDressing', dust=0.25, low_grime=0.0)
    flyers = [('rent', 0.18, 0.24, draw_rent), ('guitar', 0.15, 0.21, draw_guitar), ('cat', 0.18, 0.24, draw_cat), ('clean', 0.14, 0.18, draw_clean),
              ('keys', 0.127, 0.076, draw_keys_found), ('sale', 0.16, 0.12, draw_sale), ('tax', 0.12, 0.16, draw_tax)]
    for key, w, h, fn in flyers:
        k.panel(key, w, h, fn, density=1.5)
    cards = [('ca', "DEL'S TAILORING", 'ALTERATIONS · HEMS · ZIPPERS', '555-0161', 0xf4f2ea, 0x7a1f1f),
             ('cb', 'NORTHSIDE PLUMBING', '24 HOUR EMERGENCY', '555-0177', 0xffffff, 0x1f4f8f),
             ('cc', 'J. KIM  ·  NOTARY', 'DOCUMENTS · AFFIDAVITS', '555-0104', 0xe9e4d4, 0x2a2a2a)]
    for key, a, b, c, g, i in cards:
        k.panel(key, 0.089, 0.051, p_bcard(a, b, c, g, i), density=1.8)
    k.panel('open24', 0.66, 0.17, draw_open24)
    k.panel('attendant', 0.36, 0.12, draw_attendant)
    k.panel('nodye', 0.28, 0.20, draw_nodye)
    k.panel('notresp', 0.30, 0.10, draw_notresp)
    k.atlas.render()
    rnd = random.Random(8)
    x0, z0, x1, z1 = BOARD
    # The cork board in its aluminum frame.
    rect_pts = lambda r: [(r[0], r[1]), (r[2], r[1]), (r[2], r[3]), (r[0], r[3])]  # noqa: E731
    k.add(front_plate('frame', rect_pts(BOARD), [rect_pts((x0 + 0.02, z0 + 0.02, x1 - 0.02, z1 - 0.02))], -0.022, 0.022), 'alu')
    k.add(bx('cork', x0 + 0.018, x1 - 0.018, -0.016, -0.001, z0 + 0.018, z1 - 0.018), 'cork')
    k.add(bx('backer', x0 + 0.01, x1 - 0.01, -0.006, 0.0, z0 + 0.01, z1 - 0.01), 'particle')
    yc = -0.016
    layout = [('rent', -0.62, 1.55, -2.0), ('guitar', -0.42, 1.585, 1.5), ('cat', -0.205, 1.53, 2.5), ('clean', -0.005, 1.585, -1.5), ('keys', -0.035, 1.30, 3.0),
              ('sale', -0.30, 1.265, -3.0), ('tax', -0.63, 1.265, 2.0), ('ca', -0.47, 1.245, -6.0), ('cb', -0.155, 1.70, 4.0), ('cc', 0.05, 1.70, -3.5)]
    pin_cols = [0xc4161c, 0xf2c230, 0x1f6fd0, 0x2f9a4a, 0xf4f2ea]
    for i, (key, cx, cz, rot) in enumerate(layout):
        p = k.atlas.panels[key]
        w, h = p['w'] / 1000.0, p['h'] / 1000.0
        a = math.radians(rot)
        u, v = Vector((math.cos(a), 0.0, math.sin(a))), Vector((-math.sin(a), 0.0, math.cos(a)))
        lift = 0.0006 + 0.0005 * i
        curl = rnd.uniform(0.002, 0.006)
        if key == 'guitar':
            k.decal(key, (cx, yc, cz), u=tuple(u), v=tuple(v), lift=lift, outline=[(0.0, 0.25), (1.0, 0.25), (1.0, 1.0), (0.0, 1.0)])
            missing = {1, 2, 5}
            for t in range(8):
                if t in missing:
                    continue
                s0, s1 = t / 8 + 0.006, (t + 1) / 8 - 0.006
                k.decal(key, (cx, yc, cz), u=tuple(u), v=tuple(v), lift=lift + 0.0002 * (t % 3), outline=[(s0, 0.0), (s1, 0.0), (s1, 0.25), (s0, 0.25)])
        else:
            k.decal(key, (cx, yc, cz), u=tuple(u), v=tuple(v), lift=lift, nu=3, nv=3,
                    disp=lambda s, t, c=curl: c * (max(0.0, 0.35 - t) / 0.35) ** 2 * (0.4 + abs(2 * s - 1)))
        corners = (-1,) if key in ('ca', 'cb', 'cc', 'keys') else (-1, 1)
        col = pin_cols[i % len(pin_cols)]
        for sgn in corners:
            pin = Vector((cx, yc, cz)) + v * (h / 2 - 0.007) + u * sgn * (w / 2 - 0.007)
            k.add(disc_y('pin_head', pin.x, yc - lift - 0.0005, pin.z, 0.0042, 0.007, 16), 'gloss', col)
            k.add(disc_y('pin_cap', pin.x, yc - lift - 0.0075, pin.z, 0.0055, 0.0025, 16), 'gloss', col)
    # The signs: OPEN 24 HOURS over the board; ATTENDANT ON DUTY, NO DYEING, NOT RESPONSIBLE beside it.
    signs = [('open24', (x0 + x1) / 2, 1.93, 0.66, 0.17, 0xf6f3ea, 0.004), ('attendant', RIGHT_X, 1.86, 0.36, 0.12, 0x1e4f8f, 0.003),
             ('nodye', RIGHT_X, 1.55, 0.28, 0.20, 0xf6f3ea, 0.0015), ('notresp', RIGHT_X, 1.30, 0.30, 0.10, 0xf2c230, 0.003)]
    for key, cx, cz, w, h, col, t in signs:
        k.add(bx('sign', cx - w / 2, cx + w / 2, -t, 0.0, cz - h / 2, cz + h / 2, min(t * 0.4, 0.0015)), 'plastic' if key != 'nodye' else 'alu', col)
        k.decal(key, (cx, -t, cz), w=w - 0.002, h=h - 0.002, lift=0.0003)
        for sx in (-1, 1):
            k.add(disc_y('screw', cx + sx * (w / 2 - 0.012), -t, cz + (h / 2 - 0.012 if key == 'nodye' else 0.0), 0.0035, 0.0015, 12), 'chrome')
            if key == 'nodye':
                k.add(disc_y('screw', cx + sx * (w / 2 - 0.012), -t, cz - (h / 2 - 0.012), 0.0035, 0.0015, 12), 'chrome')
    return [k.finish()]


# ================================================================== SM_WF_TV

TV_C = Vector((0.0, -0.14, 2.12))
TV_TILT = 12.0


def tv():
    k = Kit('wftv', 'SM_WF_TV', dust=0.6, low_grime=0.0)
    k.panel('logo', 0.06, 0.012, p_label('VOLTA', 0x111213, 0xb9bcc0, 'Black', tracking=1.6), density=3.0)
    k.atlas.render()
    M = Matrix.Translation(TV_C) @ Matrix.Rotation(math.radians(TV_TILT), 4, 'X')
    parts_tv = []
    sw, sh = 0.704, 0.398
    ring = front_plate('bezel', core.rounded_rect(-0.37, -0.22, 0.37, 0.22, 0.004, 3), [[(-sw / 2, -sh / 2), (sw / 2, -sh / 2), (sw / 2, sh / 2), (-sw / 2, sh / 2)]],
                       -0.015, 0.006)
    parts_tv.append((ring, 'gloss', 0x111213))
    parts_tv.append((bx('body', -0.368, 0.368, -0.009, 0.015, -0.218, 0.218, 0.004), 'plastic', 0x161718))
    parts_tv.append((parts.rbox('hump', (0.0, 0.036, -0.02), (0.50, 0.045, 0.30), 0.02, 3), 'plastic', 0x1a1a1b))
    parts_tv.append((bx('vesa', -0.10, 0.10, 0.058, 0.064, -0.12, 0.08, 0.002), 'powder', 0x1c1c1d))
    for o, fin, col in parts_tv:
        o.data.transform(M)
        k.add(o, fin, col)
    screen = quad('screen', [(-sw / 2, -0.0105, -sh / 2), (sw / 2, -0.0105, -sh / 2), (sw / 2, -0.0105, sh / 2), (-sw / 2, -0.0105, sh / 2)])
    me = screen.data
    uv = me.uv_layers.new(name='UVMap')
    for poly in me.polygons:
        for li in poly.loop_indices:
            co = me.vertices[me.loops[li].vertex_index].co
            uv.data[li].uv = ((co.x + sw / 2) / sw, (co.z + sh / 2) / sh)
    screen.data.transform(M)
    k.special(screen, 'WF_TVScreen', locked=True)
    lg = Matrix.Translation(TV_C) @ Matrix.Rotation(math.radians(TV_TILT), 4, 'X')
    k.decal('logo', (0.0, -0.015, -0.2095), lift=0.0003, matrix=lg)
    # The bracket: a wall plate, two arms, the tilt knuckle.
    k.add(bx('wall_plate', -0.09, 0.09, -0.008, 0.0, 1.97, 2.25, 0.004, 3), 'powder', 0x1c1c1d)
    for sx in (-0.06, 0.06):
        a = Vector((sx, -0.008, 2.10))
        b = M @ Vector((sx, 0.061, -0.02))
        k.add(tube('arm', [tuple(a), tuple(b)], 0.011, 10), 'powder', 0x1c1c1d)
    kn = M @ Vector((0.0, 0.064, -0.02))
    k.add(parts.rod('knuckle', (kn.x - 0.09, kn.y, kn.z), (kn.x + 0.09, kn.y, kn.z), 0.014, 16), 'powder', 0x1c1c1d)
    for sx in (-0.07, 0.07):
        k.add(disc_y('bolt', sx, -0.008, 2.21, 0.006, 0.004, 12), 'chrome')
        k.add(disc_y('bolt', sx, -0.008, 2.01, 0.006, 0.004, 12), 'chrome')
    # The cables looping down to an outlet box.
    k.add(bx('outlet_box', 0.21, 0.29, -0.045, 0.0, 1.66, 1.78, 0.004, 3), 'plastic', 0xe6e3da)
    k.add(bx('plug', 0.236, 0.264, -0.068, -0.045, 1.692, 1.728, 0.004, 3), 'plastic', 0x161616)
    k.add(bx('coax_jack', 0.243, 0.257, -0.058, -0.045, 1.745, 1.765, 0.003), 'chrome')
    c0 = M @ Vector((0.12, 0.05, -0.16))
    c1 = M @ Vector((0.16, 0.05, -0.16))
    k.add(tube('power', core.catmull_rom([tuple(c0), (c0.x + 0.01, c0.y + 0.01, c0.z - 0.12), (0.17, -0.07, 1.62), (0.24, -0.075, 1.64), (0.25, -0.07, 1.71)], 6),
               0.0032, 8), 'rubber', 0x121212)
    k.add(tube('coax', core.catmull_rom([tuple(c1), (c1.x + 0.01, c1.y + 0.01, c1.z - 0.08), (0.21, -0.06, 1.68), (0.25, -0.062, 1.75)], 6), 0.003, 8),
          'rubber', 0x1a1a1a)
    k.grime = []
    return [k.finish()]


# ================================================================== build, bake, assemble

BUILDERS = {'washer': washer, 'dryer': dryer, 'table': fold_table, 'chairs': chairs, 'change': change_machine, 'vend': soap_vend, 'cart': cart,
            'counter': counter, 'door': lambda: door_frame() + door_leaf(), 'wall': wall_dressing, 'tv': tv}


def build():
    core.reset()
    random.seed(1812)
    make_unbaked()
    out = []
    for key in WHICH:
        if key in BUILDERS:
            out += BUILDERS[key]()
    for o in out:
        o.data.name = o.name
    return out


_SPLIT = {}


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


def bake_parts(objs):
    """Each mesh bakes without its unbaked slots (glass, drum glow, screen): they're split off, hidden from the bake
    (so they don't shade what's behind them) and joined back after it."""
    _SPLIT.clear()
    for o in objs:
        idx = [i for i, s in enumerate(o.material_slots) if s.material and s.material.name in UNBAKED]
        if not idx:
            continue
        dup = o.copy()
        dup.data = o.data.copy()
        core.link(dup)
        dup.name = o.name + '__unbaked'
        _delete_faces(o, lambda f: f.material_index in idx)
        _delete_faces(dup, lambda f: f.material_index not in idx)
        _prune_slots(o)
        _prune_slots(dup)
        dup.hide_render = True
        _SPLIT[o.name] = dup
    return list(objs)


def assemble(objs):
    out = []
    for o in objs:
        dup = _SPLIT.get(o.name)
        name = o.name
        if dup is not None:
            dup.hide_render = False
            o = core.join([o, dup], name)
            o.name = name
            o.data.name = name
        out.append(o)
    by = {o.name: o for o in out}
    if 'SM_WF_BackDoor_Frame' in by and 'SM_WF_BackDoor_Leaf' in by:
        frame, leaf = by['SM_WF_BackDoor_Frame'], by['SM_WF_BackDoor_Leaf']
        a, b = frame.copy(), leaf.copy()
        a.data, b.data = frame.data.copy(), leaf.data.copy()
        core.link(a)
        core.link(b)
        b.data.transform(Matrix.Translation(LEAF_PIVOT))
        door = core.join([a, b], 'SM_WF_BackDoor')
        door.name = 'SM_WF_BackDoor'
        door.data.name = 'SM_WF_BackDoor'
        by['SM_WF_BackDoor'] = door
    return [by[n] for n in MESHES if n in by]


# ================================================================== review

def _mat(name, hexc, rough=0.5, metal=0.0):
    m = core.Mat(name)
    m.set('Base Color', H(hexc))
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    return m


def _floor_mat():
    m = core.Mat('rv_wf_tile')
    tc = m.node('ShaderNodeTexCoord')
    ch = m.node('ShaderNodeTexChecker', Scale=1.0 / 0.3048 * 0.5 * 2.0)
    ch.inputs['Color1'].default_value = H(0xcbc6b8)
    ch.inputs['Color2'].default_value = H(0xb3ad9e)
    m.link(tc.outputs['Object'], ch.inputs['Vector'])
    n = noise(m, tc.outputs['Object'], scale=3.0, detail=4.0)
    col = m.mix(mul(m, n, 0.25), ch.outputs['Color'], H(0x8c8676))
    m.set('Base Color', col)
    m.set('Roughness', add(m, 0.28, mul(m, n, 0.2)))
    return m


def _plane(name, size, loc, rot, mat):
    bpy.ops.mesh.primitive_plane_add(size=1.0, location=loc, rotation=rot)
    o = bpy.context.view_layer.objects.active
    o.name = name
    o.scale = (size[0], size[1], 1.0)
    core.assign(o, mat)
    return o


def _shot(cam, path, loc, target, lens, w=720, h=720, samples=64):
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
    sc.render.filepath = path
    sc.render.image_settings.file_format = 'PNG'
    sc.render.image_settings.color_mode = 'RGB'
    bpy.ops.render.render(write_still=True)
    return path


def _compose(name, paths, cols):
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
        r, c = i // cols, i % cols
        y0 = Hh - (r + 1) * h - r * gap
        x0 = c * (w + gap)
        out[y0:y0 + h, x0:x0 + w] = im
    path = os.path.join(core.REVIEW_DIR, f'{name}.jpg')
    img = bpy.data.images.new(f'rv_{name}', W, Hh, alpha=False)
    img.pixels.foreach_set(out.ravel())
    img.filepath_raw = path
    img.file_format = 'JPEG'
    img.save(quality=90)
    bpy.data.images.remove(img)
    print(f'[wash_fold] review sheet {os.path.relpath(path, core.ROOT)}')
    return path


# (mesh, label, camera position, target, lens mm): positions in the mesh's own frame, the mesh at the origin.
SHOTS = {
    'SM_WF_Washer': [((-1.1, -2.0, 1.55), (0.0, 0.0, 0.62), 38), ((0.25, -0.75, 1.30), (0.04, -0.38, 0.98), 45), ((-0.35, -0.85, 0.70), (0.0, -0.38, 0.50), 40)],
    'SM_WF_DryerStack': [((-1.3, -2.7, 1.65), (0.0, 0.0, 0.98), 35), ((0.35, -0.95, 1.30), (0.0, -0.40, 0.98), 40), ((-0.15, -1.0, 1.62), (0.0, -0.3, 1.38), 40)],
    'SM_WF_FoldTable': [((-1.6, -2.8, 1.75), (0.0, 0.0, 0.55), 35), ((1.05, -0.95, 1.45), (0.62, -0.05, 0.9), 40), ((-1.6, -1.1, 0.75), (-1.0, -0.2, 0.5), 35)],
    'SM_WF_Chairs': [((-1.1, -2.7, 1.45), (0.0, 0.0, 0.48), 35), ((0.75, -1.15, 1.05), (0.3, 0.0, 0.5), 40)],
    'SM_WF_ChangeMachine': [((-1.0, -2.1, 1.65), (0.0, 0.0, 0.95), 35), ((0.25, -0.8, 1.45), (0.0, -0.2, 1.25), 40), ((-0.3, -0.75, 1.0), (0.0, -0.2, 0.86), 40)],
    'SM_WF_SoapVend': [((-1.0, -2.3, 1.65), (0.0, 0.0, 0.95), 35), ((0.2, -0.9, 1.3), (-0.08, -0.2, 1.0), 40), ((0.55, -0.85, 1.15), (0.25, -0.2, 1.0), 40)],
    'SM_WF_Cart': [((-1.3, -2.1, 1.6), (0.0, 0.0, 0.85), 35), ((0.45, -0.85, 1.95), (0.0, 0.0, 1.62), 40)],
    'SM_WF_Counter': [((-1.4, -2.8, 1.8), (0.0, 0.0, 0.8), 35), ((0.75, -1.0, 1.6), (0.35, -0.1, 1.12), 40), ((-1.3, -1.05, 1.55), (-0.72, -0.05, 1.15), 40),
                      ((0.7, 1.7, 1.75), (0.25, 0.1, 0.85), 35)],
    'SM_WF_BackDoor': [((-1.0, -2.8, 1.45), (0.0, 0.0, 1.05), 35), ((0.25, -0.75, 1.5), (0.02, -0.02, 1.38), 40), ((0.7, 2.0, 1.4), (0.0, 0.0, 1.1), 35)],
    'SM_WF_WallDressing': [((-0.3, -2.5, 1.6), (0.0, 0.0, 1.55), 35), ((-0.45, -0.95, 1.55), (-0.33, 0.0, 1.45), 40), ((0.75, -0.95, 1.6), (0.58, 0.0, 1.6), 40)],
    'SM_WF_TV': [((-0.9, -2.1, 1.55), (0.0, -0.1, 2.0), 35), ((0.3, -1.0, 1.7), (0.05, -0.12, 2.0), 40)],
}
WALL_MOUNTED = {'SM_WF_WallDressing', 'SM_WF_TV'}
NO_WALL = {'SM_WF_BackDoor', 'SM_WF_Counter'}


def detail_review(objs):
    """Close looks at each mesh under the laundromat's fluorescent tubes, on its tile floor against its pale wall."""
    sc = bpy.context.scene
    saved = {o.name: (o.location.copy(), o.rotation_euler.copy(), o.hide_render) for o in objs}
    temp = []
    tmp_dir = os.path.join(core.BUILD_DIR, 'review', 'wash_fold')
    os.makedirs(tmp_dir, exist_ok=True)
    try:
        floor = _plane('rv_floor', (30.0, 30.0), (0.0, 0.0, 0.0), (0.0, 0.0, 0.0), _floor_mat())
        wall_m = _mat('rv_wall', 0xd5dfdc, 0.85)
        wall = _plane('rv_wall', (30.0, 6.0), (0.0, 1.0, 3.0), (math.radians(90.0), 0.0, 0.0), wall_m)
        temp += [floor, wall]
        lights = []
        for x in (-1.6, 0.0, 1.6):
            for y in (-1.6, 0.2):
                d = bpy.data.lights.new('rv_tube', 'AREA')
                d.shape = 'RECTANGLE'
                d.size, d.size_y = 1.2, 0.18
                d.energy = 60.0
                d.color = (0.9, 0.96, 1.0)
                o = core.link(bpy.data.objects.new('rv_tube', d))
                o.location = (x, y, 2.95)
                lights.append(o)
        temp += lights
        sc.world.color = (0.03, 0.032, 0.036)
        sc.view_settings.view_transform = 'AgX'
        sc.view_settings.exposure = 0.0
        cd = bpy.data.cameras.new('rv_cam')
        cam = core.link(bpy.data.objects.new('rv_cam', cd))
        cd.clip_start = 0.02
        cd.clip_end = 100.0
        sc.camera = cam
        temp.append(cam)
        paths = []
        for o in objs:
            shots = SHOTS.get(o.name)
            if not shots:
                continue
            for p in objs:
                p.hide_render = p is not o
                p.location = (0.0, 0.0, 0.0)
                p.rotation_mode = 'XYZ'
                p.rotation_euler = (0.0, 0.0, 0.0)
            bpy.context.view_layer.update()
            lo, hi = core.bounds([o])
            wall.location.y = 0.0 if o.name in WALL_MOUNTED else hi.y + 0.03
            wall.hide_render = o.name in NO_WALL
            for i, (loc, tgt, lens) in enumerate(shots):
                paths.append(_shot(cam, os.path.join(tmp_dir, f'{o.name}_{i}.png'), loc, tgt, lens))
        if paths:
            _compose('wash_fold_detail', paths, 3)
    finally:
        for o in temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        sc.camera = None
        for n, (loc, rot, hid) in saved.items():
            o = bpy.data.objects.get(n)
            if o is not None:
                o.location, o.rotation_euler, o.hide_render = loc, rot, hid
        sc.view_settings.exposure = 0.0


# The standard sheet: the kit stood up as a corner of the laundromat: (x, y, yaw) per copy. Washers, dryers, the
# changer and the soap vend along the back wall, the door at its end; the fold table and the cart in the middle, the
# chairs along the left facing in, the counter at the front right.
LAYOUT = {'SM_WF_Washer': [(-2.45, 1.125, 0.0), (-1.75, 1.125, 0.0), (-1.05, 1.125, 0.0)], 'SM_WF_DryerStack': [(-0.30, 1.10, 0.0), (0.50, 1.10, 0.0)],
          'SM_WF_ChangeMachine': [(1.25, 1.30, 0.0)], 'SM_WF_SoapVend': [(1.95, 1.28, 0.0)], 'SM_WF_BackDoor': [(3.05, 1.605, 0.0)],
          'SM_WF_WallDressing': [(-1.75, 1.50, 0.0)], 'SM_WF_TV': [(1.60, 1.50, 0.0)], 'SM_WF_FoldTable': [(-1.25, -0.25, 0.0)],
          'SM_WF_Chairs': [(-3.35, -0.35, 90.0)], 'SM_WF_Cart': [(0.55, -0.15, -20.0)], 'SM_WF_Counter': [(2.65, -0.75, 0.0)],
          'SM_WF_BackDoor_Frame': [(4.6, 1.6, 0.0)], 'SM_WF_BackDoor_Leaf': [(4.0, 0.5, -60.0)]}


def pose_for_review(objs):
    global REVIEW_VIEWS
    if os.environ.get('WF_REVIEW', '1') != '0':
        detail_review(objs)
    shown = list(objs)
    for o in objs:
        spots = LAYOUT.get(o.name, [(0.0, 0.0, 0.0)])
        for k, (x, y, yaw) in enumerate(spots):
            d = o
            if k:
                d = o.copy()
                core.link(d)
                shown.append(d)
            d.location = (x, y, 0.0)
            d.rotation_mode = 'XYZ'   # glTF imports (build.py --review-only) come in as quaternions
            d.rotation_euler = (0.0, 0.0, math.radians(yaw))
    bpy.context.view_layer.update()
    lo, hi = core.bounds(shown)
    size = max((hi - lo).length, 1e-3)
    c = (lo + hi) / 2
    REVIEW_VIEWS = [('room', -10, 20, 0.85, (c.x, c.y, 0.9)), ('angle', -38, 24, 0.7, (c.x, c.y, 0.8)),
                    ('detail', -20, 12, 0.26, (-1.75, 0.2, 0.95))]
    if size < 1e-2:
        REVIEW_VIEWS = None
