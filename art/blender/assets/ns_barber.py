"""SM_NS_Barber_*: Fifth St. Barbers, the walk-up's north bay at 1818 Fifth St., Northside (spec world_realism_spec.md
section 4.5; the shop itself section 5.1). Lonnie Pruitt's shop: a 1995 shop in a 1905 cast-iron front.

This module also holds the SHOP KIT that ns_diner.py and ns_handprops.py import (finishes chosen per face by an
attribute and baked per slot, a print atlas with decals, the unbaked slots the game drives, the bake split and the
review rig). Everything above the line "the barbershop" is the kit.

FRAMES (spec 0.4). SITE meshes: Blender (0, 0, 0) = the stage origin, placed in Unreal at the stage's transform with
yaw 180, so stage (X, Y, Z) cm = (-100x, +100y, +100z). They are modelled round a local anchor (the opening's middle
on the front plane at the floor, stage (0, 3365, 0)) and moved to the stage frame after the bake, so the procedural
textures never see big coordinates. PROP meshes are in their own frame, front toward Blender -X, placed at L with
yaw F + 180 to face stage yaw F. The view from Blender -X looking +X is the view from the far sidewalk: north (+y) on
the left, so print on the front reads left to right toward -y, and reads correctly in game (spec 0.4).

THE ROOM (stage cm): storefront opening X 0 plane, Y 3112..3618, Z 0..335; the interior X -35..-900, Y 3125..3610,
floor Z 0, the pressed-tin ceiling Z 330. The door at the opening's south end, Y 3130..3225, recessed 60 cm, hinged
on the south jamb and swinging in. The backbar (station) runs along the north (party) wall, the two chairs face it;
the waiting chairs run along the south wall facing north; the shampoo sink is at the back of the north wall; the
back wall (X -900) has the door to the back room (not enterable).

SLOTS. Baked slots are named M_<Mesh>_<i> in Unreal in the order the build log prints ("[ns_barber] <mesh>: ... slots
[...]"). Unbaked slots keep their Blender names:
  ns_glass        [glass] -> M_NS_Glass. A dark tint at alpha 0.15 in the glb (glTF BLEND). Nanite off on any mesh
                  that has it.
  ns_mirror       [mirror] -> M_NS_Mirror. UV0 0..1 across each panel as seen (u right, v up); the glb's base color
                  is the desilvering mask (white silver, dark where the backing has gone at the edges), and the same
                  mask is written to unreal/Art/Textures/NS/T_NS_MirrorEdge.png (linear, R = silver left).
  ns_barberpole   [runtime] -> M_NS_BarberPole: the pole's striped insert. UV0: u 0..1 once round the insert
                  (counter-clockwise from above, from the wall side), v = meters up from the insert's foot (0..0.62).
                  The band is frac(2 (u + 1.5 v / 0.62) + time) (red, white, blue, white; two of each per turn, a turn
                  and a half over the insert); the glb carries a still of them.
  ns_screen_*     [screen]: UV0 0..1 across the face as seen (u right, v up), drawn by core. The glb carries a
                  placeholder of what core will draw. Per screen below.
Glow meshes (spec 0.5 [glow:<family>]) are separate meshes with one white emissive slot (parts.glow_object); the game
sets their color and level.
TILED BAKES (the kit's tile_slot): a periodic surface (pressed tin, subway tile, terrazzo, quarry tile) is baked once on
a proxy square of its period and its faces carry UVs that run past 0..1, so the texture repeats at full resolution
(about 1.2 mm a texel on the tin and the wainscot): keep those textures' address mode at Wrap (Unreal's default).
Everything that varies (a tile's tone, crazing, rust at a lap) is a function of the tile's id within the period, so
the repeat has no seam.
SHADING: hard parts get smooth shading with edges past the mesh's angle kept sharp and face-area weighted normals; parts
added with soft=True (cloth: the coats, the capes, the rag; food) keep plain smooth normals with no sharp edges.

THE MESHES (sizes W x D x H, meters):
- SM_NS_Barber_Front (SITE, P0, 40k): the storefront in the opening. A tiled bulkhead (bottle-green 5 cm glazed
  squares over a black base course, a bullnose cap), the painted wood sash (dark green enamel, brass glazing beads)
  round two display panes and a glazed return into the door's recess, the recess (60 cm deep, its floor white hex
  mosaic in a black border, a brass threshold), the door frame, the two-light transom with gold leaf
  (FIFTH ST. BARBERS / EST. 1995 over the display, WALK-INS WELCOME over the door; read from the street; from inside
  the reverse of the gilding shows, mirrored, as it does), the grille's hood and guides, and the neon OPEN's black
  backing grid and chains in the south pane; inside, the window ledge (a snake plant in terracotta, pomade tins, a
  tonic bottle, a CASH ONLY card) and the city license in the glass's corner. The gilding is real lettering geometry
  (sharp at any distance). Slots: 0 frame (sash, hood, guides, sill, gilding), 1 tile (bulkhead, recess floor),
  2 prints (labels and cards).
- SM_NS_Barber_Front_Glass (SITE, P0): the display, return, transom and door-sidelight glass. [glass] only.
- SM_NS_Barber_Front_GlowNeon (SITE, P0): the OPEN tubes (single-stroke letters, 17 cm) hanging in the south pane,
  facing the street (stage Y 3325, Z 146..163); the jumpers between letters are blacked-out tube in the Front. The
  game colors it red (0xff2a1c) and turns it off when the shop is shut.
- SM_NS_Barber_Door (PROP, P0, 8k): the door leaf, 0.93 x 0.045 x 2.28 (the opening 0.95 x 2.30). Origin on its hinge
  axis at the floor (the leaf's inside face); the leaf runs along Blender +Y from the hinge, its outside face toward
  -X. Place it at stage (-64.5, 3132, 0) with yaw 180 (its outside faces the street, stage yaw 0); open it by yawing about
  Z: the leaf swings in (toward stage -X) for positive Unreal yaw. A glass panel (ns_glass), a push bar inside and a
  brass pull outside, a brass kick plate, a deadbolt, a hanging OPEN/CLOSED card on a chain (ns_screen_doorsign:
  the card's outside face u 0..0.5, its inside face u 0.5..1, v 0.45..1; and the hours card taped inside the glass at
  1.24..1.42 m, u 0..1, v 0..0.4, read from outside: core draws shop::HoursLine there). The bell's bracket is on the
  frame (in SM_NS_Barber_Front).
  Slots: 0 baked, 1 ns_glass, 2 ns_screen_doorsign.
- SM_NS_DoorBell (PROP, P0): the brass shop bell on its coil spring. Origin at the spring's root (the end of the
  bracket's arm on the door frame inside: stage (-72, 3205, 237)); the bell hangs along -Z to Z 224, where the opening
  leaf's top rail strikes it. The
  game swings it about its origin when the door opens (as the Wash & Fold's bell).
- SM_NS_Barber_Grille (PROP, P0, 6k): the roll-down link grille. Origin at the top middle of its curtain (the hood's
  underside), stage (+9, 3365, 307); front toward -X (the street), yaw 0. 5.02 wide, 3.07 tall (down to Z 0). The
  game scales it in Z (1 = down, about 0.02 = rolled up into the hood, which is in SM_NS_Barber_Front).
- SM_NS_Barber_Pole (PROP, P0, 6k) + SM_NS_Barber_Pole_GlowWarm (the globe): a classic wall pole on two brackets.
  Origin on the wall face under the pole at the ground (the pole's axis is 0.30 out along -X); place it on the north
  pier's cast-iron face, stage (+16.5, 3652, 0), yaw 0 (where the old pole was). The insert from Z 1.88 to 2.50, the
  globe's middle at Z 2.78. Slots: 0 baked (chrome caps, brackets, wall plate), 1 ns_barberpole, 2 ns_glass (the
  glass sleeve over the insert).
- SM_NS_Barber_Shell (SITE, P0, 60k): the room. The 30 cm black and white checker floor, the mint (0x9fd3c7) subway
  tile wainscot to 1.20 m with a bullnose cap and a black cove base, cream walls, the pressed-tin ceiling at 3.30 with
  its crown molding, the back wall's door (painted, a PRIVATE plate, a closer) to the back room. Slots: 0 floor,
  1 tile, 2 walls (paint, tin, moldings, the door), 3 prints.
- SM_NS_Barber_Chair (PROP, P0, 45k; two instances): a hydraulic barber chair, 0.68 x 1.02 x 1.27. Origin on the
  floor at the pedestal's axis; the sitter faces -X. Sockets (chair frame, meters): Seat (0.0, 0, 0.55) the seat's
  top middle (the seat mark, 0.55 m); Head (0.19, 0, 1.25) the head's middle against the headrest (the barber's
  working ring); Feet (-0.50, 0, 0.20) on the footrest; Arms (-0.03, +-0.30, 0.79) the armrests' pads. Placements:
  chair 1 at stage (-260, 3460, 0), chair 2 at
  (-460, 3460, 0), both yaw 270 (facing north, stage yaw 90, toward the mirror).
- SM_NS_Barber_Station (SITE, P0, 90k): the backbar on the north wall, stage X -155..-565: base cabinets with drawers
  (walnut-stained, brass pulls) under a marble top at 0.88 m, a marble shelf at 1.10 m under the mirrors, the
  hot-towel cabinet, the sterilizer, a jar of blue disinfectant (no label) with combs in it, clippers on hooks with
  coiled cords, combs, brushes, spray bottles, talc, a straight razor and strop, a neck-strip dispenser, a small radio,
  a style book, tonics on the shelf. Slots: 0 baked, 1 prints, 2 ns_glass (the jar's glass: Nanite off).
- SM_NS_Barber_Mirror (SITE, P0, 2k): two beveled mirror panels over the station (stage X -165..-355 and -365..-555,
  Z 112..232, their faces at stage Y 3607.5, framed by the station's beads and pilasters). Slot: ns_mirror.
- SM_NS_Barber_Waiting (SITE, P0, 30k): four chrome-tube waiting chairs with oxblood vinyl seats (0.45 m) along the
  south wall, a side table with magazines and a folded sports page. Stations barber-bench1..4: stage
  (-150, 3160, 45), (-205, 3160, 45), (-310, 3160, 45), (-365, 3160, 45), seat tops, each facing north (yaw 90).
- SM_NS_Barber_Wall (SITE, P0, 30k): framed photos (Lonnie and young Dee, 1998; Fifth St in 1978; the shop license),
  a coat rack with two coats and the spare cape, the TV on its bracket (ns_screen_tv, 16:9), the price board
  (ns_screen_prices, 3:4, core-drawn from the barber's menu), the wall clock (dial baked, no hands: the game adds them
  as the store's clock; the dial's middle at stage (-893.4, 3368, 262), its face toward stage +X (yaw 0), radius
  17 cm; set the hands 5 mm in front of it), the hairstyle chart, a garage's calendar. The TV's middle at stage
  (-878, 3507, 240), turned 18 degrees toward the room and tipped 8 degrees down; the price board at stage
  X -235..-280, Z 150..210 on the south wall. Slots: 0 baked, 1 prints, 2 ns_screen_tv, 3 ns_screen_prices.
- SM_NS_Barber_Sink (SITE, P1, 15k): a black porcelain shampoo bowl (its neck notch to the front) on a walnut cabinet
  with a gooseneck and a sprayer, and its reclined chair facing south, at the back of the north wall (stage
  X -730..-850).
- SM_NS_Barber_Cape (PROP, P1, 6k): the pinstriped cutting cape, cloth-simulated over a seated "Average" stand-in in
  the chair. Origin at the neck's middle at the collar: place it at the chair's Seat socket + (0.125, 0, 0.60), the
  chair's yaw; the sitter faces -X.
- SM_NS_Barber_Lights (SITE, P0, 6k) + SM_NS_Barber_Lights_GlowWarm (the two milk-glass globes) +
  SM_NS_Barber_Lights_GlowCool (the two fluorescent wraparound lenses): pendants over the chairs at stage
  (-260, 3440) and (-460, 3440), globe middles at Z 246; the tube fittings over the station at Y 3555,
  X -199..-321 and -399..-521, and a third over the sink, X -730..-852 (the spec's two plus one so the back of the
  room has its own light), Z 330.

The barber's uses/stations (spec 2.10): barber-chair1 = chair 1's Head socket; barber-chair2 = chair 2's; the
barber stands at the chair's side (stage +-55 cm in X of the Head, 10 cm south). barber-mirror camera: from the mirror
(stage Y 3600, Z 150, the chair's X) toward the Head, 40 degrees. barber-door: stage (+30, 3170, 0).

NSB_ONLY=front,door,... (keys of BUILDERS) builds a subset; NSB_REVIEW=room,detail picks the extra review sheets
(art/review/ns_barber_room.jpg: the shop at night and by day from the street and inside; ns_barber_detail.jpg: close
looks). The standard sheet ns_barber.jpg lines the props up in studio light.
"""
import math
import os
import random

import bmesh
import bpy  # noqa: F401
import numpy as np
from mathutils import Matrix, Vector

from artkit import core, parts, review, texmaps
from artkit.shade import convex_edges, mix_float, noise, object_coords, ramp, scaled, smooth_less, vec
from artkit.sheet import Sheet

# =====================================================================================================================
# THE SHOP KIT (shared with ns_diner and ns_handprops)
# =====================================================================================================================

TEX_NS = os.path.join(core.ROOT, 'unreal', 'Art', 'Textures', 'NS')


def H(h, a=1.0):
    return core.hex_linear(h, a)


def lin3(h):
    c = core.hex_linear(h)
    return (c[0], c[1], c[2])


def S(X, Y, Z=0.0):
    """Stage centimeters to Blender meters (SITE frame)."""
    return (-X / 100.0, Y / 100.0, Z / 100.0)


# ------------------------------------------------------------------ geometry helpers

def set_attr(obj, name, value):
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
    """An axis-aligned box from its extents, edges rounded by r."""
    return parts.rbox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), r, seg)


def tube(name, pts, r, n=12, closed=False):
    return core.sweep(name, pts, parts.circle(r, n), closed=closed, caps=not closed)


def weld(obj, dist=1e-6):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=dist)
    bm.to_mesh(obj.data)
    bm.free()


def lathe_solid(name, profile, matrix=None, segments=48):
    """A closed solid of revolution from an (r, z) profile that starts and ends on the axis."""
    obj = core.lathe(name, profile, segments)
    weld(obj)
    core.orient_normals(obj)
    if matrix is not None:
        obj.data.transform(matrix)
    return obj


def lathe_shell(name, profile, matrix=None, segments=48, toward=None):
    """An open surface of revolution (a bowl, a shade), normals out (or toward a lathe-space direction)."""
    obj = core.lathe(name, profile, segments)
    core.orient_normals(obj, toward)
    if matrix is not None:
        obj.data.transform(matrix)
    return obj


def bend(points, radius, steps=6, min_turn=0.0):
    """A 3D polyline with its corners rounded (bent tube, wire). Corners turning less than min_turn degrees (the points
    of an already smooth curve) are kept as they are."""
    P = [Vector(p) for p in points]
    out = [P[0]]
    keep = math.pi - math.radians(max(min_turn, 0.06))
    for i in range(1, len(P) - 1):
        a, c, b = P[i - 1], P[i], P[i + 1]
        d0, d1 = a - c, b - c
        l0, l1 = d0.length, d1.length
        d0.normalize()
        d1.normalize()
        ang = math.acos(max(-1.0, min(1.0, d0.dot(d1))))
        if ang > keep or ang < 1e-3:
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
    V = N.cross(U).normalized()
    m = Matrix((U, V, N)).transposed().to_4x4()
    m.translation = Vector(origin)
    return m


def quad(name, corners):
    bm = bmesh.new()
    vs = [bm.verts.new(c) for c in corners]
    bm.faces.new(vs)
    return core.mesh_object(name, bm)


def grid_plane(name, x0, x1, y0, y1, z, cell=1.5, up=True):
    """A floor or ceiling as separate square-ish pieces (each its own UV island, so a long room packs into a square
    texture): pieces split on multiples of `cell` (put the cell on a tile multiple so the seams fall in the grout)."""
    xs = [x0] + [x0 + cell * k for k in range(1, int((x1 - x0) / cell) + 1) if x0 + cell * k < x1 - 1e-4] + [x1]
    ys = [y0] + [y0 + cell * k for k in range(1, int((y1 - y0) / cell) + 1) if y0 + cell * k < y1 - 1e-4] + [y1]
    bm = bmesh.new()
    for i in range(len(xs) - 1):
        for j in range(len(ys) - 1):
            a, b, c, d = (xs[i], ys[j]), (xs[i + 1], ys[j]), (xs[i + 1], ys[j + 1]), (xs[i], ys[j + 1])
            vs = [bm.verts.new((p[0], p[1], z)) for p in ((a, b, c, d) if up else (a, d, c, b))]
            bm.faces.new(vs)
    return core.mesh_object(name, bm)


def wall_plane(name, p0, p1, z0, z1, normal, seg=2.2, holes=()):
    """A vertical wall face from p0 to p1 (2D points), z0..z1, facing `normal` (2D), cut into pieces no longer than
    seg (one UV island each). holes: (t0, t1, hz0, hz1) spans along the wall (meters from p0) left open."""
    P0, P1 = Vector((p0[0], p0[1], 0.0)), Vector((p1[0], p1[1], 0.0))
    L = (P1 - P0).length
    d = (P1 - P0) / L
    cuts = sorted({0.0, L, *[t for h in holes for t in h[:2]], *[seg * k for k in range(1, int(L / seg) + 1) if seg * k < L - 1e-4]})
    zs_all = sorted({z0, z1, *[z for h in holes for z in h[2:] if z0 < z < z1]})
    bm = bmesh.new()
    N = Vector((normal[0], normal[1], 0.0))
    for a, b in zip(cuts, cuts[1:]):
        for za, zb in zip(zs_all, zs_all[1:]):
            mid, zm = (a + b) / 2, (za + zb) / 2
            if any(h[0] <= mid <= h[1] and h[2] <= zm <= h[3] for h in holes):
                continue
            pa, pb = P0 + d * a, P0 + d * b
            vs = [bm.verts.new((pa.x, pa.y, za)), bm.verts.new((pb.x, pb.y, za)), bm.verts.new((pb.x, pb.y, zb)), bm.verts.new((pa.x, pa.y, zb))]
            f = bm.faces.new(vs)
            f.normal_update()
            if f.normal.dot(N) < 0:
                f.normal_flip()
    return core.mesh_object(name, bm)


def apply_mod(obj, mod):
    core.select_only([obj])
    bpy.ops.object.modifier_apply(modifier=mod.name)


def solidify(obj, t, offset=-1.0):
    mod = obj.modifiers.new('solid', 'SOLIDIFY')
    mod.thickness = t
    mod.offset = offset
    apply_mod(obj, mod)
    return obj


def blob(name, center, size, seed=1, detail=0.18, subdiv=3, flatten=0.0, smooth=0):
    """A soft lump (a folded coat, a towel roll): an icosphere scaled to size and pushed about by noise."""
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=1.0)
    Pn = np.array([v.co[:] for v in bm.verts])
    n = texmaps.value_noise(Pn * 0.5 + seed * 3.1, 0.6, seed=seed, octaves=3) - 0.5
    for v, a in zip(bm.verts, n):
        co = v.co.normalized() * (1.0 + detail * a * 2.0)
        if flatten > 0.0 and co.z < 0.0:
            co.z *= (1.0 - flatten)
        v.co = Vector((center[0] + co.x * size[0] / 2, center[1] + co.y * size[1] / 2, center[2] + co.z * size[2] / 2))
    for _ in range(smooth):
        bmesh.ops.smooth_vert(bm, verts=bm.verts[:], factor=0.5, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    obj = core.mesh_object(name, bm)
    obj.data.shade_smooth()
    return obj


def cushion(name, x0, x1, y0, y1, z0, z1, r, crown=0.01, sub=2, matrix=None):
    """An upholstered pad: a rounded box with a soft crown on top, subdivided smooth."""
    obj = bx(name, x0, x1, y0, y1, z0, z1, r * 0.5, 1)
    core.subdivide(obj, sub)
    if crown:
        cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
        hx, hy = (x1 - x0) / 2, (y1 - y0) / 2
        for v in obj.data.vertices:
            if v.co.z > (z0 + z1) / 2:
                k = max(0.0, 1.0 - ((v.co.x - cx) / hx) ** 2) * max(0.0, 1.0 - ((v.co.y - cy) / hy) ** 2)
                v.co.z += crown * k
    if matrix is not None:
        obj.data.transform(matrix)
    obj.data.shade_smooth()
    return obj


# ------------------------------------------------------------------ finishes (one per face, by the ns_fin attribute)

FIN = {}
FINISH = {}
METALLIC = set()


def finish_kind(name, metal=False):
    def deco(fn):
        if name not in FIN:
            FIN[name] = len(FIN) + 1
        FINISH[name] = fn
        if metal:
            METALLIC.add(name)
        return fn
    return deco


class Ctx:
    def __init__(self, m, kit, slot):
        self.m, self.kit, self.slot = m, kit, slot
        self.tc, self.sep = object_coords(m)
        self.P = self.tc.outputs['Object']
        self.x, self.y, self.z = self.sep.outputs['X'], self.sep.outputs['Y'], self.sep.outputs['Z']
        geo = m.node('ShaderNodeNewGeometry')
        ns = m.node('ShaderNodeSeparateXYZ')
        m.link(geo.outputs['Normal'], ns.inputs['Vector'])
        self.nx, self.ny, self.nz = ns.outputs['X'], ns.outputs['Y'], ns.outputs['Z']
        self.fin = attr(m, 'ns_fin')
        self.col = attr(m, 'ns_col', 'Color')
        self.aux = attr(m, 'ns_aux')
        self._ao = {}

    def edges(self, distance=0.003, samples=8, breakup_scale=90.0, gain=4.0):
        """convex_edges, with the traced occlusion shared by every finish in the material (two radii: fine and coarse):
        a mixed material evaluates every finish's graph at every point, and each traced AO node multiplies the bake's
        cost."""
        m = self.m
        key = 'fine' if distance <= 0.0022 else 'coarse'
        if key not in self._ao:
            n = m.node('ShaderNodeAmbientOcclusion', Distance=0.0015 if key == 'fine' else 0.0040)
            n.inside = True
            n.only_local = True
            n.samples = 8
            self._ao[key] = n.outputs['AO']
        breakup = m.node('ShaderNodeTexNoise', Scale=breakup_scale, Detail=5.0)
        m.link(self.tc.outputs['Object'], breakup.inputs['Vector'])
        convex = m.math('MULTIPLY', m.math('SUBTRACT', 0.93, self._ao[key]), gain, clamp=True)
        return m.math('MULTIPLY', convex, m.math('MULTIPLY', m.math('SUBTRACT', breakup.outputs['Fac'], 0.3), 2.2, clamp=True))


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
    """col moved toward a lighter version of itself (a gamma lift in linear space keeps the hue: mixing toward white
    would wash a dark red or a syrup brown out to pink-grey)."""
    g = m.node('ShaderNodeGamma')
    g.inputs['Gamma'].default_value = 0.55
    if isinstance(col, tuple):
        g.inputs['Color'].default_value = col
    else:
        m.link(col, g.inputs['Color'])
    return m.mix(amount, col, g.outputs['Color'])


def darken(m, col, f):
    return m.mix(1.0, col, (f, f, f, 1.0), blend='MULTIPLY')


def isolines(m, vector, scale, width, detail=1.0):
    """Thin wandering lines (scratches, cracks, veins): where a noise crosses its middle."""
    n = noise(m, vector, scale=scale, detail=detail)
    return ramp(m, m.math('ABSOLUTE', m.math('SUBTRACT', n, 0.5)), width, 0.0)


def cell_rand(m, vector, size, seed=0.0):
    """A random 0..1 per cell of a grid of `size` (a float or an (sx, sy, sz) tuple)."""
    d = m.node('ShaderNodeVectorMath')
    d.operation = 'DIVIDE'
    m.link(vector, d.inputs[0])
    d.inputs[1].default_value = size if isinstance(size, (tuple, list)) else (size, size, size)
    f = m.node('ShaderNodeVectorMath')
    f.operation = 'FLOOR'
    m.link(d.outputs[0], f.inputs[0])
    a = m.node('ShaderNodeVectorMath')
    a.operation = 'ADD'
    m.link(f.outputs[0], a.inputs[0])
    a.inputs[1].default_value = (seed * 7.13, seed * 3.71, seed * 1.37)
    w = m.node('ShaderNodeTexWhiteNoise')
    w.noise_dimensions = '3D'
    m.link(a.outputs[0], w.inputs['Vector'])
    return w.outputs['Value']


def mul_vec(m, v, k):
    n = m.node('ShaderNodeVectorMath')
    n.operation = 'SCALE'
    m.link(v, n.inputs[0])
    n.inputs['Scale'].default_value = k
    return n.outputs[0]


def voronoi(m, vector, scale, feature='F1', out='Distance', randomness=1.0):
    v = m.node('ShaderNodeTexVoronoi', Scale=scale)
    v.feature = feature
    v.inputs['Randomness'].default_value = randomness
    m.link(vector, v.inputs['Vector'])
    return v.outputs[out]


def path_mask(m, c, paths):
    """1 along floor traffic paths: (x0, y0, x1, y1, half width) segments, softened and broken up."""
    out = None
    for (x0, y0, x1, y1, w) in paths:
        dx, dy = x1 - x0, y1 - y0
        L2 = max(dx * dx + dy * dy, 1e-6)
        t = m.math('MINIMUM', m.math('MAXIMUM', m.math('DIVIDE', add(m, mul(m, sub(m, c.x, x0), dx), mul(m, sub(m, c.y, y0), dy)), L2), 0.0), 1.0)
        px = sub(m, c.x, add(m, x0, mul(m, t, dx)))
        py = sub(m, c.y, add(m, y0, mul(m, t, dy)))
        d = m.math('SQRT', add(m, mul(m, px, px), mul(m, py, py)))
        k = smooth_less(m, d, w * 0.55, w * 0.45)
        out = k if out is None else m.math('MAXIMUM', out, k)
    if out is None:
        return 0.0
    return mul(m, out, ramp(m, noise(m, c.P, scale=3.0, detail=4.0), 0.25, 0.65))


def bevel_h(m, edge, width, depth):
    """A rounded tile edge: 0 on the face, falling by `depth` (mm) toward the joint over `width` (m)."""
    k = ramp(m, edge, 0.0, width)
    return mul(m, sub(m, m.math('SQRT', m.math('MAXIMUM', sub(m, 1.0, mul(m, sub(m, 1.0, k), sub(m, 1.0, k))), 0.0)), 1.0), depth)


@finish_kind('paint')
def f_paint(c):
    """Interior latex, eggshell: stipple, roller laps, touch-up patches, scuffs low."""
    m = c.m
    stip = noise(m, c.P, scale=380.0, detail=2.0)
    roll = noise(m, scaled(m, c.sep, 9.0, 9.0, 1.5), detail=2.0)
    patch = ramp(m, noise(m, c.P, scale=0.9, detail=3.0), 0.62, 0.68)
    col = m.mix(mul(m, stip, 0.18), c.col, darken(m, c.col, 0.92))
    col = m.mix(mul(m, roll, 0.10), col, lighten(m, c.col, 0.10))
    col = m.mix(mul(m, patch, 0.35), col, lighten(m, c.col, 0.06))
    scuff = mul(m, ramp(m, c.z, 1.05, 0.3), ramp(m, noise(m, scaled(m, c.sep, 14.0, 14.0, 90.0), detail=4.0), 0.62, 0.74))
    col = m.mix(mul(m, scuff, 0.45), col, H(0x5e5a52))
    # years of smoke and cooking: a faint yellowing toward the ceiling, uneven
    age = mul(m, ramp(m, c.z, 2.0, 3.6), ramp(m, noise(m, scaled(m, c.sep, 0.6, 0.6, 1.5), detail=3.0), 0.3, 0.8))
    col = m.mix(mul(m, age, 0.22), col, darken(m, H(0xc8b48a), 0.9))
    rough = add(m, 0.62, mul(m, stip, 0.08), mul(m, patch, -0.12), mul(m, scuff, -0.1))
    return col, rough, 0.0, add(m, mul(m, stip, 0.025), mul(m, roll, 0.015))


@finish_kind('enamel')
def f_enamel(c):
    """Gloss enamel on wood or steel (sash, trim, fascias): brush marks, orange peel, chips to the undercoat."""
    m = c.m
    peel = noise(m, c.P, scale=420.0, detail=1.0)
    brush = noise(m, scaled(m, c.sep, 3.0, 3.0, 260.0), detail=2.0)
    edge = c.edges( distance=0.003, samples=12, breakup_scale=60.0, gain=5.0)
    chip = ramp(m, edge, 0.5, 0.62)
    col = m.mix(mul(m, brush, 0.12), c.col, lighten(m, c.col, 0.08))
    col = m.mix(chip, col, H(0x6e6252))
    dull = ramp(m, noise(m, c.P, scale=2.0, detail=3.0), 0.55, 0.8)
    rough = add(m, 0.22, mul(m, peel, 0.05), mul(m, dull, 0.12), mul(m, chip, 0.3))
    return col, rough, 0.0, add(m, mul(m, peel, 0.01), mul(m, brush, 0.01), mul(m, chip, -0.05))


@finish_kind('tin')
def f_tin(c):
    """A pressed-tin ceiling, 61 cm panels: a raised border, a bead row, an inner square and a rosette, painted cream
    over many coats, yellowed, with grime in the recesses and a little rust at the laps."""
    m = c.m
    Pn = 0.6096
    ox, oy = c.kit.tin_origin
    xx, yy = sub(m, c.x, ox), sub(m, c.y, oy)
    u = m.math('FRACT', m.math('DIVIDE', xx, Pn))
    v = m.math('FRACT', m.math('DIVIDE', yy, Pn))
    # Everything that varies is a function of the panel (its id mod 2) and the point inside it, so a 2 x 2 panel bake
    # repeats seamlessly (the kit's tiled slots).
    pid_x = m.math('FLOORED_MODULO', m.math('FLOOR', m.math('DIVIDE', xx, Pn)), 2.0)
    pid_y = m.math('FLOORED_MODULO', m.math('FLOOR', m.math('DIVIDE', yy, Pn)), 2.0)
    loc = vec(m, add(m, mul(m, u, Pn), mul(m, pid_x, 3.17)), add(m, mul(m, v, Pn), mul(m, pid_y, 1.73)), 0.0)
    a = mul(m, m.math('ABSOLUTE', sub(m, u, 0.5)), 2.0)
    b = mul(m, m.math('ABSOLUTE', sub(m, v, 0.5)), 2.0)
    r = m.math('MAXIMUM', a, b)
    rr = mul(m, m.math('SQRT', add(m, mul(m, sub(m, u, 0.5), sub(m, u, 0.5)), mul(m, sub(m, v, 0.5), sub(m, v, 0.5)))), 2.0)

    def band(x, c0, w):
        return smooth_less(m, m.math('ABSOLUTE', sub(m, x, c0)), w, w * 0.6)

    h = mul(m, band(r, 0.9, 0.035), 1.6)                       # the raised border
    h = add(m, h, mul(m, smooth_less(m, r, 0.985, 0.012), -0.6))  # the lap at the panel's edge
    # Beads round the square at r 0.80: along each side a row of domes.
    along = mix_float(m, m.math('GREATER_THAN', a, b), u, v)
    s_ = m.math('FRACT', mul(m, along, 14.0))
    bd = m.math('SQRT', add(m, mul(m, mul(m, sub(m, s_, 0.5), 1.0 / 14.0), mul(m, sub(m, s_, 0.5), 1.0 / 14.0)),
                            mul(m, mul(m, sub(m, r, 0.8), 0.5), mul(m, sub(m, r, 0.8), 0.5))))
    h = add(m, h, mul(m, smooth_less(m, bd, 0.012, 0.008), 1.4))
    h = add(m, h, mul(m, band(r, 0.62, 0.025), 1.2))           # the inner square
    ang = m.math('ARCTAN2', sub(m, v, 0.5), sub(m, u, 0.5))
    petal = mul(m, add(m, m.math('COSINE', mul(m, ang, 8.0)), 1.0), 0.5)
    ros = mul(m, smooth_less(m, sub(m, rr, mul(m, petal, 0.14)), 0.16, 0.03), 1.5)
    h = add(m, h, ros, mul(m, smooth_less(m, rr, 0.06, 0.02), 0.8))
    h = add(m, h, mul(m, band(rr, 0.42, 0.02), 0.7))
    col = m.mix(ramp(m, noise(m, loc, scale=2.0, detail=3.0), 0.3, 0.8), c.col, darken(m, c.col, 0.9))
    low = ramp(m, h, 0.5, 0.0)
    col = m.mix(mul(m, low, 0.25), col, H(0x8a7a5c))
    lap = smooth_less(m, r, 0.985, 0.012)
    rust = mul(m, lap, ramp(m, noise(m, loc, scale=4.0, detail=3.0), 0.68, 0.76))
    col = m.mix(mul(m, rust, 0.4), col, H(0x8a5a32))
    drip = mul(m, ramp(m, noise(m, loc, scale=1.6, detail=3.0), 0.6, 0.85), 0.18)
    col = m.mix(drip, col, H(0xc4b48c))
    rough = add(m, 0.42, mul(m, low, 0.12), mul(m, rust, 0.3))
    return col, rough, 0.0, add(m, h, mul(m, noise(m, loc, scale=300.0, detail=1.0), 0.03))


@finish_kind('checker')
def f_checker(c):
    """The barber's floor: 30 cm black and white tile, grout 0x6b675f, waxed; worn grey along the paths, a few
    corners chipped, the white ones yellowed where the mop doesn't reach."""
    m = c.m
    T = 0.30
    ox, oy = c.kit.tile_origin
    xx, yy = sub(m, c.x, ox), sub(m, c.y, oy)
    fu = m.math('FRACT', m.math('DIVIDE', xx, T))
    fv = m.math('FRACT', m.math('DIVIDE', yy, T))
    eu = mul(m, m.math('MINIMUM', fu, sub(m, 1.0, fu)), T)
    ev = mul(m, m.math('MINIMUM', fv, sub(m, 1.0, fv)), T)
    e = m.math('MINIMUM', eu, ev)
    grout = smooth_less(m, e, 0.0016, 0.0004)
    par = m.math('FLOORED_MODULO', add(m, m.math('FLOOR', m.math('DIVIDE', xx, T)), m.math('FLOOR', m.math('DIVIDE', yy, T))), 2.0)
    rnd = cell_rand(m, vec(m, xx, yy, 0.0), (T, T, 1.0), 1.0)
    white = m.mix(rnd, H(0xe9e5da), H(0xddd6c4))
    black = m.mix(rnd, H(0x121212), H(0x1c1b1a))
    col = m.mix(par, white, black)
    wear = path_mask(m, c, c.kit.paths)
    scuff = mul(m, wear, ramp(m, noise(m, scaled(m, c.sep, 90.0, 90.0, 1.0), detail=3.0), 0.5, 0.75))
    col = m.mix(mul(m, mul(m, scuff, par), 0.16), col, H(0x4a4844))
    col = m.mix(mul(m, mul(m, scuff, sub(m, 1.0, par)), 0.12), col, H(0xb5ab95))
    yell = mul(m, sub(m, 1.0, par), ramp(m, noise(m, c.P, scale=1.2, detail=3.0), 0.55, 0.8))
    col = m.mix(mul(m, yell, 0.3), col, H(0xcbbd98))
    heel = mul(m, isolines(m, scaled(m, c.sep, 9.0, 9.0, 1.0), 1.0, 0.004, 2.0), mul(m, sub(m, 1.0, par), ramp(m, noise(m, c.P, scale=2.0), 0.5, 0.7)))
    col = m.mix(mul(m, heel, 0.5), col, H(0x2a2826))
    # Chipped corners: a few tiles lose a corner to the grey body.
    cu = mul(m, m.math('MINIMUM', fu, sub(m, 1.0, fu)), T)
    cv = mul(m, m.math('MINIMUM', fv, sub(m, 1.0, fv)), T)
    cd = m.math('SQRT', add(m, mul(m, cu, cu), mul(m, cv, cv)))
    chipped = mul(m, m.math('GREATER_THAN', cell_rand(m, vec(m, xx, yy, 0.0), (T, T, 1.0), 5.0), 0.93),
                  smooth_less(m, add(m, cd, mul(m, sub(m, noise(m, c.P, scale=120.0), 0.5), 0.01)), 0.016, 0.002))
    col = m.mix(chipped, col, H(0x8d877b))
    gcol = m.mix(ramp(m, noise(m, c.P, scale=4.0, detail=3.0), 0.3, 0.8), H(0x6b675f), H(0x3e3a34))
    col = m.mix(grout, col, gcol)
    rough = add(m, 0.18, mul(m, scuff, 0.18), mul(m, rnd, 0.05), mul(m, ramp(m, noise(m, c.P, scale=5.0, detail=3.0), 0.4, 0.8), 0.06))
    rough = mix_float(m, chipped, rough, 0.8)
    rough = mix_float(m, grout, rough, 0.88)
    h = add(m, bevel_h(m, e, 0.003, 0.5), mul(m, grout, -1.2), mul(m, chipped, -0.6))
    return col, rough, 0.0, h


@finish_kind('subway')
def f_subway(c):
    """Glazed subway tile, 15 x 7.5 cm in running bond from z0 = kit.tile_z0: pillowed edges, a wavy glaze, a tone each,
    faint crazing, grout darker toward the floor."""
    m = c.m
    W, Hh = 0.15, 0.075
    along = add(m, c.x, c.y)
    zz = sub(m, c.z, c.kit.tile_z0)
    row = m.math('FLOOR', m.math('DIVIDE', zz, Hh))
    off = mul(m, m.math('FLOORED_MODULO', row, 2.0), 0.5 * W)
    fu = m.math('FRACT', m.math('DIVIDE', add(m, along, off), W))
    fv = m.math('FRACT', m.math('DIVIDE', zz, Hh))
    eu = mul(m, m.math('MINIMUM', fu, sub(m, 1.0, fu)), W)
    ev = mul(m, m.math('MINIMUM', fv, sub(m, 1.0, fv)), Hh)
    e = m.math('MINIMUM', eu, ev)
    grout = smooth_less(m, e, 0.0011, 0.0003)
    # Tile ids wrap every 8 tiles and the noise runs in each tile's own coordinates, so a 1.2 m bake repeats seamlessly.
    tidx = m.math('FLOORED_MODULO', m.math('FLOOR', m.math('DIVIDE', add(m, along, off), W)), 8.0)
    tid = vec(m, tidx, row, 0.0)
    loc = vec(m, add(m, mul(m, fu, W), mul(m, tidx, 0.731)), add(m, mul(m, fv, Hh), mul(m, row, 0.419)), 0.0)
    rnd = cell_rand(m, tid, (1.0, 1.0, 1.0), 2.0)
    col = m.mix(rnd, darken(m, c.col, 0.94), lighten(m, c.col, 0.07))
    craze = mul(m, ramp(m, voronoi(m, loc, 90.0, 'DISTANCE_TO_EDGE'), 0.012, 0.0), m.math('GREATER_THAN', cell_rand(m, tid, (1.0, 1.0, 1.0), 9.0), 0.7))
    col = m.mix(mul(m, craze, 0.3), col, darken(m, c.col, 0.72))
    gcol = m.mix(ramp(m, c.z, 0.5, 0.0), H(0xb9b5aa), H(0x7c776c))
    gcol = m.mix(ramp(m, noise(m, loc, scale=60.0, detail=3.0), 0.5, 0.8), gcol, H(0x8f897c))
    col = m.mix(grout, col, gcol)
    wave = noise(m, mul_vec(m, loc, 22.0), detail=2.0)
    rough = add(m, 0.08, mul(m, rnd, 0.04), mul(m, craze, 0.05))
    rough = mix_float(m, grout, rough, 0.85)
    h = add(m, bevel_h(m, e, 0.006, 0.9), mul(m, grout, -0.8), mul(m, wave, 0.12), mul(m, craze, -0.02))
    return col, rough, 0.0, h


@finish_kind('chrome', metal=True)
def f_chrome(c):
    m = c.m
    sm = ramp(m, noise(m, c.P, scale=14.0, detail=4.0), 0.5, 0.82)
    pit = ramp(m, noise(m, c.P, scale=700.0, detail=2.0), 0.74, 0.8)
    scr = isolines(m, scaled(m, c.sep, 40.0, 40.0, 40.0), 1.0, 0.004, 2.0)
    col = m.mix(mul(m, pit, 0.7), H(0xd6d9dc), H(0x6e6a62))
    rough = add(m, 0.05, mul(m, sm, 0.1), mul(m, pit, 0.3), mul(m, scr, 0.06))
    return col, rough, 1.0, add(m, mul(m, pit, -0.01), mul(m, scr, -0.004))


@finish_kind('stainless', metal=True)
def f_stainless(c):
    """Brushed stainless along X (aux 1: along Y, 2: along Z): fingerprints, wipe marks, fine scratches."""
    m = c.m
    bx_ = noise(m, scaled(m, c.sep, 4.0, 500.0, 900.0), detail=3.0)
    by_ = noise(m, scaled(m, c.sep, 500.0, 4.0, 900.0), detail=3.0)
    bz_ = noise(m, scaled(m, c.sep, 900.0, 500.0, 4.0), detail=3.0)
    br = mix_float(m, is_k(m, c.aux, 1), mix_float(m, is_k(m, c.aux, 2), bx_, bz_), by_)
    smudge = ramp(m, noise(m, c.P, scale=12.0, detail=4.0), 0.56, 0.82)
    prints = ramp(m, noise(m, c.P, scale=45.0, detail=3.0, distortion=1.2), 0.62, 0.72)
    scr = isolines(m, scaled(m, c.sep, 30.0, 30.0, 30.0), 1.0, 0.004, 2.0)
    col = m.mix(br, H(0x9a9ea2), H(0xbec2c5))
    col = m.mix(mul(m, smudge, 0.14), col, H(0x7a7d80))
    rough = add(m, 0.24, mul(m, br, 0.06), mul(m, smudge, 0.07), mul(m, prints, 0.08), mul(m, scr, 0.08))
    return col, rough, 1.0, add(m, mul(m, br, 0.004), mul(m, scr, -0.004))


@finish_kind('porcelain')
def f_porcelain(c):
    """Porcelain enamel on cast iron (a barber chair's base and arms, a sink): gloss, faintly crazed and yellowed, chipped
    to black iron on the edges, rust bleeding from the chips."""
    m = c.m
    edge = c.edges( distance=0.003, samples=12, breakup_scale=70.0, gain=5.0)
    chip = ramp(m, edge, 0.55, 0.66)
    halo = ramp(m, edge, 0.38, 0.55)
    age = ramp(m, noise(m, c.P, scale=3.0, detail=3.0), 0.4, 0.8)
    craze = ramp(m, voronoi(m, c.P, 60.0, 'DISTANCE_TO_EDGE'), 0.01, 0.0)
    col = m.mix(mul(m, age, 0.35), c.col, H(0xd9ceb4))
    col = m.mix(mul(m, craze, 0.12), col, H(0x8c8676))
    col = m.mix(mul(m, sub(m, halo, chip), 0.5), col, H(0x8a5a32))
    col = m.mix(chip, col, H(0x1c1b1a))
    rough = add(m, 0.07, mul(m, craze, 0.04), mul(m, ramp(m, noise(m, c.P, scale=900.0, detail=3.0, distortion=2.0), 0.62, 0.7), 0.1))
    rough = mix_float(m, chip, rough, 0.6)
    return col, rough, 0.0, add(m, mul(m, chip, -0.15), mul(m, craze, -0.01))


@finish_kind('vinyl')
def f_vinyl(c):
    """Upholstery vinyl: a pebble grain, worn glossy where people sit (kit.polish), cracked along the edges to the
    cream scrim, faint creases."""
    m = c.m
    gr = voronoi(m, c.P, 1400.0, 'F1', 'Distance')
    grain = ramp(m, gr, 0.0, 0.6)
    edge = c.edges( distance=0.006, samples=10, breakup_scale=30.0, gain=4.0)
    crack = mul(m, isolines(m, scaled(m, c.sep, 40.0, 40.0, 40.0), 1.0, 0.035, 3.0), ramp(m, edge, 0.25, 0.6))
    crease = isolines(m, c.P, 18.0, 0.03, 3.0)
    col = m.mix(mul(m, grain, 0.12), c.col, darken(m, c.col, 0.85))
    col = m.mix(mul(m, crease, 0.25), col, darken(m, c.col, 0.65))
    col = m.mix(mul(m, edge, 0.15), col, lighten(m, c.col, 0.15))
    col = m.mix(crack, col, H(0xcdbb9a))
    rough = add(m, 0.42, mul(m, grain, 0.08), mul(m, crack, 0.4), mul(m, crease, 0.05))
    return col, rough, 0.0, add(m, mul(m, grain, 0.06), mul(m, crack, -0.25), mul(m, crease, -0.1))


@finish_kind('wood')
def f_wood(c):
    """Stained, lacquered hardwood: the grain runs along X (aux 0), Y (1) or Z (2); lighter, duller worn edges."""
    m = c.m

    def grain(sx, sy, sz):
        w = m.node('ShaderNodeTexWave', Scale=6.0, Distortion=7.0, Detail=3.0)
        w.wave_type = 'RINGS'
        m.link(scaled(m, c.sep, sx, sy, sz), w.inputs['Vector'])
        return w.outputs['Fac']

    gx, gy, gz = grain(0.08, 2.0, 2.0), grain(2.0, 0.08, 2.0), grain(2.0, 2.0, 0.08)
    g = mix_float(m, is_k(m, c.aux, 1), mix_float(m, is_k(m, c.aux, 2), gx, gz), gy)
    fine = mix_float(m, is_k(m, c.aux, 1), mix_float(m, is_k(m, c.aux, 2), noise(m, scaled(m, c.sep, 4.0, 400.0, 400.0), detail=2.0),
                                                         noise(m, scaled(m, c.sep, 400.0, 400.0, 4.0), detail=2.0)),
                     noise(m, scaled(m, c.sep, 400.0, 4.0, 400.0), detail=2.0))
    late = ramp(m, g, 0.35, 0.75)
    col = m.mix(mul(m, late, 0.55), c.col, darken(m, c.col, 0.6))
    col = m.mix(mul(m, fine, 0.2), col, darken(m, c.col, 0.8))
    edge = ramp(m, c.edges( distance=0.003, samples=10, breakup_scale=50.0), 0.4, 0.65)
    col = m.mix(mul(m, edge, 0.5), col, lighten(m, c.col, 0.3))
    rough = add(m, 0.32, mul(m, late, 0.06), mul(m, edge, 0.25), mul(m, ramp(m, noise(m, c.P, scale=4.0, detail=3.0), 0.5, 0.8), 0.1))
    return col, rough, 0.0, add(m, mul(m, late, -0.03), mul(m, fine, 0.02))


@finish_kind('marble')
def f_marble(c):
    """White veined marble, honed and polished, water rings and a stain or two."""
    m = c.m
    d = noise(m, c.P, scale=1.5, detail=6.0, distortion=2.5)
    vein = ramp(m, m.math('ABSOLUTE', sub(m, noise(m, vec(m, add(m, c.x, mul(m, d, 0.6)), c.y, c.z), scale=2.2, detail=5.0), 0.5)), 0.018, 0.0)
    vein2 = ramp(m, m.math('ABSOLUTE', sub(m, noise(m, vec(m, c.y, add(m, c.z, mul(m, d, 0.4)), c.x), scale=5.0, detail=4.0), 0.5)), 0.01, 0.0)
    cloud = noise(m, c.P, scale=4.0, detail=4.0)
    col = m.mix(mul(m, cloud, 0.25), c.col, darken(m, c.col, 0.9))
    col = m.mix(mul(m, vein, 0.7), col, H(0x7d8288))
    col = m.mix(mul(m, vein2, 0.35), col, H(0xa2a6aa))
    stain = ramp(m, noise(m, c.P, scale=6.0, detail=4.0), 0.68, 0.74)
    col = m.mix(mul(m, stain, 0.3), col, H(0xb8a888))
    rough = add(m, 0.1, mul(m, stain, 0.12), mul(m, ramp(m, noise(m, c.P, scale=30.0, detail=3.0), 0.55, 0.75), 0.1))
    return col, rough, 0.0, mul(m, vein, -0.02)


@finish_kind('brass', metal=True)
def f_brass(c):
    """Brass: dark patina in the hollows, rubbed bright on the edges, fingerprints."""
    m = c.m
    edge = ramp(m, c.edges( distance=0.002, samples=10, breakup_scale=80.0), 0.3, 0.6)
    pat = ramp(m, noise(m, c.P, scale=25.0, detail=4.0), 0.4, 0.75)
    col = m.mix(mul(m, pat, 0.6), H(0xb08d4a), H(0x5e4a26))
    col = m.mix(edge, col, H(0xd9b870))
    rough = add(m, 0.28, mul(m, pat, 0.2), mul(m, edge, -0.12))
    return col, rough, mix_float(m, mul(m, pat, 0.5), 1.0, 0.7), mul(m, pat, -0.01)


@finish_kind('iron', metal=True)
def f_iron(c):
    """Blackened cast iron or steel (a griddle, hardware): seasoned, scraped bright here and there, a little rust."""
    m = c.m
    n = noise(m, c.P, scale=40.0, detail=4.0)
    scr = isolines(m, scaled(m, c.sep, 30.0, 4.0, 30.0), 1.0, 0.01, 2.0)
    rust = ramp(m, noise(m, c.P, scale=12.0, detail=4.0), 0.68, 0.76)
    col = m.mix(mul(m, n, 0.4), H(0x1a1918), H(0x2a2522))
    col = m.mix(mul(m, scr, 0.6), col, H(0x6a6864))
    col = m.mix(mul(m, rust, 0.7), col, H(0x5a3018))
    return col, add(m, 0.45, mul(m, n, 0.15), mul(m, scr, -0.15), mul(m, rust, 0.3)), mix_float(m, rust, 0.7, 0.0), mul(m, n, 0.02)


@finish_kind('plastic')
def f_plastic(c):
    m = c.m
    g = noise(m, c.P, scale=1800.0, detail=1.0)
    edges = mul(m, c.edges( distance=0.0012, samples=10, breakup_scale=220.0), 0.6)
    col = m.mix(mul(m, edges, 0.5), c.col, lighten(m, c.col, 0.22))
    return col, sub(m, add(m, 0.45, mul(m, g, 0.06)), mul(m, edges, 0.1)), 0.0, mul(m, g, 0.004)


@finish_kind('gloss')
def f_gloss(c):
    m = c.m
    scratch = ramp(m, noise(m, c.P, scale=900.0, detail=4.0, distortion=2.0), 0.62, 0.7)
    smear = ramp(m, noise(m, c.P, scale=14.0, detail=3.0), 0.5, 0.8)
    return c.col, add(m, 0.08, mul(m, scratch, 0.12), mul(m, smear, 0.1)), 0.0, 0.0


@finish_kind('rubber')
def f_rubber(c):
    m = c.m
    g = noise(m, c.P, scale=900.0, detail=1.0)
    dust = ramp(m, noise(m, c.P, scale=20.0, detail=3.0), 0.5, 0.8)
    return m.mix(mul(m, dust, 0.3), c.col, H(0x5a5650)), add(m, 0.78, mul(m, g, 0.08)), 0.0, mul(m, g, 0.01)


@finish_kind('paper')
def f_paper(c):
    m = c.m
    fib = noise(m, scaled(m, c.sep, 900.0, 300.0, 900.0), detail=2.0)
    return m.mix(mul(m, fib, 0.15), c.col, darken(m, c.col, 0.85)), add(m, 0.78, mul(m, fib, 0.1)), 0.0, mul(m, fib, 0.004)


@finish_kind('cloth')
def f_cloth(c):
    m = c.m
    period = 0.0018
    wa = m.math('SINE', mul(m, add(m, c.x, c.z), 2 * math.pi / period))
    wb = m.math('SINE', mul(m, add(m, c.y, c.z), 2 * math.pi / period))
    weave = mul(m, add(m, mul(m, wa, wb), 1.0), 0.5)
    heather = noise(m, c.P, scale=600.0, detail=2.0)
    fold = ramp(m, noise(m, c.P, scale=25.0, detail=3.0), 0.3, 0.7)
    col = m.mix(mul(m, heather, 0.4), c.col, lighten(m, c.col, 0.12))
    col = m.mix(mul(m, fold, 0.2), col, darken(m, c.col, 0.75))
    return col, 0.9, 0.0, add(m, mul(m, weave, 0.06), mul(m, heather, 0.03))


@finish_kind('nylon')
def f_nylon(c):
    """A barber's cape: woven nylon with a sheen, hairs caught in it."""
    m = c.m
    sheen = noise(m, c.P, scale=30.0, detail=3.0)
    hairs = mul(m, isolines(m, scaled(m, c.sep, 200.0, 200.0, 200.0), 1.0, 0.006, 1.0), ramp(m, noise(m, c.P, scale=8.0), 0.55, 0.7))
    col = m.mix(mul(m, sheen, 0.2), c.col, lighten(m, c.col, 0.15))
    stripe = mul(m, is_k(m, c.aux, 1), smooth_less(m, m.math('ABSOLUTE', sub(m, m.math('FRACT', m.math('DIVIDE', c.y, 0.028)), 0.5)), 0.035, 0.012))
    col = m.mix(mul(m, stripe, 0.55), col, H(0xb8b8b4))
    col = m.mix(mul(m, hairs, 0.8), col, H(0x2a1e16))
    return col, add(m, 0.38, mul(m, sheen, 0.1)), 0.0, mul(m, noise(m, c.P, scale=1500.0, detail=1.0), 0.01)


@finish_kind('glassdark')
def f_glassdark(c):
    """Glass baked opaque (jars, bottles, a radio's dial window): dark, glossy, a film of fingerprints."""
    m = c.m
    film = ramp(m, noise(m, c.P, scale=30.0, detail=4.0), 0.5, 0.8)
    return c.col, add(m, 0.04, mul(m, film, 0.1)), 0.0, 0.0


@finish_kind('liquid')
def f_liquid(c):
    m = c.m
    return c.col, add(m, 0.03, mul(m, noise(m, c.P, scale=20.0, detail=2.0), 0.03)), 0.0, 0.0


@finish_kind('ceramic')
def f_ceramic(c):
    """Glazed stoneware or china: gloss, a faint pinhole speckle, crazing, grey at a chipped rim."""
    m = c.m
    sp = ramp(m, noise(m, c.P, scale=500.0, detail=1.0), 0.7, 0.74)
    craze = ramp(m, voronoi(m, c.P, 120.0, 'DISTANCE_TO_EDGE'), 0.012, 0.0)
    col = m.mix(mul(m, sp, 0.4), c.col, darken(m, c.col, 0.7))
    col = m.mix(mul(m, craze, 0.1), col, darken(m, c.col, 0.8))
    return col, add(m, 0.07, mul(m, sp, 0.05), mul(m, ramp(m, noise(m, c.P, scale=800.0, detail=3.0, distortion=2.0), 0.62, 0.7), 0.1)), 0.0, mul(m, sp, -0.01)


@finish_kind('felt')
def f_felt(c):
    m = c.m
    n = noise(m, c.P, scale=500.0, detail=4.0)
    return m.mix(mul(m, n, 0.5), c.col, lighten(m, c.col, 0.15)), 0.95, 0.0, mul(m, n, 0.05)


@finish_kind('mosaic')
def f_mosaic(c):
    """Small unglazed mosaic: 2.5 cm hexagons (aux 0) or 5 cm glazed squares (aux 1) in c.col, white-grey grout."""
    m = c.m
    # hexagons on the floor (x, y); squares on a face (along x+y, z)
    a = mix_float(m, is_k(m, c.aux, 1), c.x, add(m, c.x, c.y))
    b = mix_float(m, is_k(m, c.aux, 1), c.y, c.z)
    Sx = 0.025                          # across the flats
    pa, pb = m.math('DIVIDE', a, Sx), m.math('DIVIDE', b, Sx)
    s3 = 1.7320508
    ax_ = sub(m, m.math('FLOORED_MODULO', pa, 1.0), 0.5)
    ay_ = sub(m, m.math('FLOORED_MODULO', pb, s3), 0.5 * s3)
    bx_ = sub(m, m.math('FLOORED_MODULO', sub(m, pa, 0.5), 1.0), 0.5)
    by_ = sub(m, m.math('FLOORED_MODULO', sub(m, pb, 0.5 * s3), s3), 0.5 * s3)
    near_a = m.math('LESS_THAN', add(m, mul(m, ax_, ax_), mul(m, ay_, ay_)), add(m, mul(m, bx_, bx_), mul(m, by_, by_)))
    gx = m.math('ABSOLUTE', mix_float(m, near_a, bx_, ax_))
    gy = m.math('ABSOLUTE', mix_float(m, near_a, by_, ay_))
    hexd = mul(m, sub(m, 0.5, m.math('MAXIMUM', gx, add(m, mul(m, gx, 0.5), mul(m, gy, 0.8660254)))), Sx)
    T = 0.05
    fu = m.math('FRACT', m.math('DIVIDE', a, T))
    fv = m.math('FRACT', m.math('DIVIDE', b, T))
    sq = mul(m, m.math('MINIMUM', m.math('MINIMUM', fu, sub(m, 1.0, fu)), m.math('MINIMUM', fv, sub(m, 1.0, fv))), T)
    e = mix_float(m, is_k(m, c.aux, 1), hexd, sq)
    grout = smooth_less(m, e, 0.0012, 0.0003)
    rnd = noise(m, c.P, scale=180.0, detail=1.0)
    col = m.mix(mul(m, rnd, 0.3), c.col, darken(m, c.col, 0.9))
    col = m.mix(grout, col, m.mix(0.55, darken(m, c.col, 0.55), H(0x6a665c)))
    rough = mix_float(m, is_k(m, c.aux, 1), 0.55, 0.12)
    rough = mix_float(m, grout, rough, 0.9)
    return col, rough, 0.0, add(m, bevel_h(m, e, 0.003, 0.4), mul(m, grout, -0.8))


METAL_BAKE = METALLIC


def body_material(kit, slot):
    """One baked slot's material: the finishes its faces use, mixed by ns_fin, then the wear they all share."""
    m = core.Mat(f'{kit.prefix}_{slot}')
    c = Ctx(m, kit, slot)
    col, rough, metal, h = H(0x808080), 0.5, 0.0, 0.0
    for kind in sorted(kit.kinds[slot], key=FIN.get):
        fc, fr, fm, fh = FINISH[kind](c)
        mk = is_k(m, c.fin, FIN[kind])
        col = m.mix(mk, col, fc)
        rough = mix_float(m, mk, rough, fr)
        metal = mix_float(m, mk, metal, fm)
        h = mix_float(m, mk, h, fh)
    ao = m.node('ShaderNodeAmbientOcclusion', Distance=kit.cavity)
    ao.only_local = True
    ao.samples = 8
    cav = ramp(m, ao.outputs['AO'], 0.85, 0.35)
    col = m.mix(mul(m, cav, 0.35), col, H(0x2e2a24))
    rough = add(m, rough, mul(m, cav, 0.08))
    if kit.low > 0.0:
        breakup = ramp(m, noise(m, c.P, scale=7.0, detail=4.0), 0.3, 0.7)
        low = mul(m, ramp(m, c.z, kit.low, 0.0), breakup)
        col = m.mix(mul(m, low, 0.45), col, H(0x332e27))
        rough = mix_float(m, mul(m, low, 0.6), rough, 0.85)
        metal = mix_float(m, mul(m, low, 0.5), metal, 0.0)
    for (cx, cy, cz), rad, amt in kit.grime:
        dx, dy, dz = sub(m, c.x, cx), sub(m, c.y, cy), sub(m, c.z, cz)
        d = m.math('SQRT', add(m, mul(m, dx, dx), mul(m, dy, dy), mul(m, dz, dz)))
        g = mul(m, mul(m, smooth_less(m, d, rad * 0.6, rad * 0.4), ramp(m, noise(m, c.P, scale=35.0, detail=3.0), 0.3, 0.7)), amt)
        col = m.mix(mul(m, g, 0.6), col, H(0x332d24))
        rough = mix_float(m, g, rough, 0.6)
    for (cx, cy, cz), rad, amt in kit.polish:
        dx, dy, dz = sub(m, c.x, cx), sub(m, c.y, cy), sub(m, c.z, cz)
        d = m.math('SQRT', add(m, mul(m, dx, dx), mul(m, dy, dy), mul(m, dz, dz)))
        g = mul(m, mul(m, smooth_less(m, d, rad * 0.55, rad * 0.45), ramp(m, noise(m, c.P, scale=18.0, detail=3.0), 0.25, 0.65)), amt)
        col = m.mix(mul(m, g, 0.12), col, darken(m, col, 0.85))
        rough = mix_float(m, g, rough, 0.22)
    if kit.dust > 0.0:
        up = mul(m, ramp(m, c.nz, 0.6, 0.95), mul(m, ramp(m, noise(m, c.P, scale=60.0, detail=5.0), 0.35, 0.9), kit.dust * 0.7))
        if not isinstance(metal, (int, float)):
            up = mul(m, up, sub(m, 1.0, mul(m, metal, 0.6)))
        col = m.mix(up, col, H(0x9a958b))
        rough = mix_float(m, up, rough, 0.9)
        metal = mix_float(m, up, metal, 0.0)
    m.set('Base Color', col)
    m.set('Roughness', rough if not isinstance(rough, (int, float)) else rough)
    m.set('Metallic', metal)
    if slot in kit.emit:
        # A [lit] slot: what glows (glass block lit from behind, a lamp's lens) bakes into the Emissive texture,
        # weighted per face by ns_aux (0 = dark).
        m.set('Emission Color', m.mix(1.0, col, vec(m, c.aux, c.aux, c.aux), blend='MULTIPLY'))
        m.set('Emission Strength', kit.emit[slot])
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
    col = m.mix(mul(m, grime, kit.print_grime), img.outputs['Color'], H(0x3a342c))
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


# ------------------------------------------------------------------ print atlas

_WIDTHS = {}


def text_w(s, text, face='Black', tracking=1.0):
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
        return self.s.text(txt, X, Y, size * self.k, color, face=face, align=align, rot=rot, tracking=tracking, metal=metal, rough=rough, alpha=alpha)

    def fit(self, txt, x, y, size, max_w, color, face='Black', align='CENTER', tracking=1.0, metal=0.0, rough=0.4, rot=0.0):
        size = min(size, max_w / text_w(self.s, txt, face, tracking))
        self.text(txt, x, y, size, color, face, align, rot, tracking, metal, rough)
        return size

    def mid(self, txt, x, cy, size, max_w, color, face='Black', align='CENTER', tracking=1.0, metal=0.0, rough=0.4):
        size = min(size, max_w / text_w(self.s, txt, face, tracking))
        self.text(txt, x, cy - 0.355 * size, size, color, face, align, 0.0, tracking, metal, rough)
        return size

    def shaded(self, txt, x, cy, size, max_w, color, outline, shade, face='Black', tracking=1.0, metal=0.0, rough=0.3, ow=0.05,
               sd=(0.06, -0.07), squeeze=1.0):
        """Signwriter's lettering: a drop shade, a keyline, the face (gold leaf on glass, a painted sign)."""
        size = min(size, max_w / (text_w(self.s, txt, face, tracking) * squeeze))
        y = cy - 0.355 * size
        objs = [self.text(txt, x + sd[0] * size, y + sd[1] * size, size, shade, face, 'CENTER', 0.0, tracking, 0.0, 0.6)]
        w = ow * size
        for i in range(12):
            a = i * math.pi / 6
            objs.append(self.text(txt, x + w * math.cos(a), y + w * math.sin(a), size, outline, face, 'CENTER', 0.0, tracking, 0.0, 0.5))
        objs.append(self.text(txt, x, y, size, color, face, 'CENTER', 0.0, tracking, metal, rough))
        if squeeze != 1.0:
            X, _ = self.P(x, 0)
            for o in objs:
                o.scale.x = squeeze
                o.location.x = X + (o.location.x - X) * squeeze
        return size

    def mesh(self, verts, faces, color, metal=0.0, rough=0.5, alpha=1.0):
        me = bpy.data.meshes.new('poly')
        me.from_pydata([(self.P(x, y)[0], self.P(x, y)[1], 0.0) for x, y in verts], [], faces)
        return self.s._add(bpy.data.objects.new('poly', me), color, metal, rough, alpha)

    def stroke(self, pts, width, color, rough=0.6, alpha=1.0):
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
            self.mesh(verts, faces, color, 0.0, rough, alpha)


class Atlas:
    """Print panels (millimeters) packed into one square image, rendered in one pass."""

    def __init__(self, name, px, gap=0.006):
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
        x0, y0, x1, y1 = self.rects[key]
        e = 0.6 / self.px
        return (x0 + e, y0 + e, x1 - e, y1 - e)


def render_image(name, w, h, px, draw):
    """A standalone printed image (a screen's placeholder, a texture): draw(sub, w, h) in millimeters."""
    s = Sheet(w, h)
    draw(Sub(s, 0.0, 0.0, 1.0), w, h)
    color, _ = s.render(name, px, samples=16)
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


# ------------------------------------------------------------------ unbaked slots

UNB = {}


def unbaked(name, maker=None):
    """The shared unbaked material `name` (made once per build)."""
    if name in UNB:
        return UNB[name]
    if maker is not None:
        mt = maker(name)
    elif name == 'ns_glass':
        g = core.Mat('ns_glass')
        g.set('Base Color', H(0x2a3230))
        g.set('Roughness', 0.02)
        g.set('Alpha', 0.15)
        g.set('Specular IOR Level', 0.6)
        mt = g.m
        mt.use_backface_culling = False
        try:
            mt.surface_render_method = 'BLENDED'
        except (AttributeError, TypeError):
            pass
    elif name == 'ns_mirror':
        g = core.Mat('ns_mirror')
        img = g.image(mirror_mask())
        g.link(img.outputs['Color'], g.bsdf.inputs['Base Color'])
        g.set('Metallic', 1.0)
        g.set('Roughness', 0.02)
        mt = g.m
        mt.use_backface_culling = True
    else:
        raise KeyError(name)
    if mt.name != name:
        raise RuntimeError(f'material name clash: wanted {name}, got {mt.name}')
    UNB[name] = mt
    return mt


def screen_material(name, image, strength=1.2, base=0x0a0b0c, rough=0.2, lit=True):
    """A [screen] slot with a placeholder of what core draws there (emissive when it's a lit display)."""
    def make(n):
        m = core.Mat(n)
        img = m.image(image)
        if lit:
            m.set('Base Color', H(base))
            m.link(img.outputs['Color'], m.bsdf.inputs['Emission Color'])
            m.set('Emission Strength', strength)
        else:
            m.link(img.outputs['Color'], m.bsdf.inputs['Base Color'])
        m.set('Roughness', rough)
        m.m.use_backface_culling = True
        return m.m
    return unbaked(name, make)


def mirror_mask():
    """T_NS_MirrorEdge: the silver left on an old mirror (UV 0..1 across a panel): spotty desilvering creeping in from
    the edges, worst at the bottom where the cleaner runs down. Also written to unreal/Art/Textures/NS."""
    N = 1024
    u = (np.arange(N) + 0.5) / N
    U, V = np.meshgrid(u, u)
    d = np.minimum(np.minimum(U, 1 - U), np.minimum(V, 1 - V) * np.where(V < 0.5, 0.75, 1.0))
    P = np.stack([U * 9.0, V * 9.0, np.zeros_like(U)], -1).reshape(-1, 3)
    n1 = texmaps.value_noise(P, 1.0, seed=3, octaves=4).reshape(N, N)
    n2 = texmaps.value_noise(P * 4.0, 1.0, seed=7, octaves=3).reshape(N, N)
    edge = np.clip((0.006 + 0.012 * n1 ** 2 - d) / 0.004, 0, 1)
    spots = np.clip((n2 - 0.84) * 10.0, 0, 1) * np.clip((0.035 - d) / 0.02, 0, 1)
    loss = np.clip(edge + spots, 0, 1)
    silver = 1.0 - 0.85 * loss
    img = np.ones((N, N, 4), np.float32)
    tint = np.stack([silver * 0.93, silver * 0.94, silver * 0.95], -1)
    tint = tint * (1.0 - 0.6 * loss[..., None]) + loss[..., None] * np.array([0.08, 0.07, 0.06]) * 0.6
    img[:, :, :3] = np.clip(tint, 0, 1)
    os.makedirs(core.BUILD_DIR, exist_ok=True)
    path = os.path.join(core.BUILD_DIR, 'textures', 'ns_mirror_mask.png')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    im = bpy.data.images.new('ns_mirror_mask', N, N, alpha=False)
    im.colorspace_settings.name = 'sRGB'
    im.pixels.foreach_set(img.ravel())
    im.filepath_raw = path
    im.file_format = 'PNG'
    im.save()
    os.makedirs(TEX_NS, exist_ok=True)
    lin = np.ones((N, N, 4), np.float32)
    lin[:, :, 0] = silver
    lin[:, :, 1] = silver
    lin[:, :, 2] = silver
    im2 = bpy.data.images.new('T_NS_MirrorEdge', N, N, alpha=False)
    im2.colorspace_settings.name = 'Non-Color'
    im2.pixels.foreach_set(lin.ravel())
    im2.filepath_raw = os.path.join(TEX_NS, 'T_NS_MirrorEdge.png')
    im2.file_format = 'PNG'
    im2.save()
    bpy.data.images.remove(im2)
    return path


# ------------------------------------------------------------------ the kit

class Kit:
    """One mesh's parts. Baked parts carry a finish (ns_fin), a tint (ns_col) and an aux value (ns_aux), and go to a
    named baked slot; decals carry atlas UVs (the print slot); unbaked parts carry a shared UNB material with their
    own UVs. `slots` lists the mesh's slot order: baked slot names, 'print', unbaked material names."""

    def __init__(self, prefix, name, slots=('body',), sizes=None, dust=0.25, low=0.10, anchor=(0.0, 0.0, 0.0), cavity=0.012,
                 print_px=1024, print_grime=0.12, log='ns'):
        self.prefix, self.name, self.slots = prefix, name, list(slots)
        self.sizes = sizes if sizes is not None else {}
        self.objs = []
        self.kinds = {}
        self.atlas = None
        self.dust, self.low, self.cavity = dust, low, cavity
        self.anchor = anchor
        self.grime, self.polish, self.paths = [], [], []
        self.tile_origin = (0.0, 0.0)
        self.tile_z0 = 0.0
        self.print_px = print_px
        self.print_grime = print_grime
        self.log = log
        self.tiled = {}
        self.emit = {}
        self.tin_origin = (0.0, 0.0)
        self.default = next((s for s in self.slots if s != 'print' and not s.startswith('ns_')), 'body')

    def tile_slot(self, slot, mode, period, origin=(0.0, 0.0), z=0.0, z_range=(0.0, 1.0), down=False):
        """Make `slot` a repeating bake: its faces get UVs that repeat over `period` (pu, pv) meters, and the slot is
        baked once on a proxy square of that size (bake_proxies), whose material the faces then share.
        mode 'xy': floors and ceilings (u = (x - ox) / pu, v = (y - oy) / pv), the proxy at height z (facing down
        with down=True). mode 'wall': u = (along - ox) / pu along the wall (x or y, whichever the face runs along),
        v = (z - z0) / (z1 - z0) over z_range; the proxy stands in the y = 0 plane facing -y."""
        self.tiled[slot] = dict(mode=mode, period=period, origin=origin, z=z, z_range=z_range, down=down)

    def add(self, obj, fin, col=0xffffff, slot=None, aux=0.0, soft=False):
        """soft=True: an organic part (cloth, food, a cushion's drape) shaded fully smooth, with no sharp edges and no
        face-area weighting (finish_shading), which would facet it."""
        slot = slot or self.default
        set_attr(obj, 'ns_fin', float(FIN[fin]))
        set_attr(obj, 'ns_col', lin3(col))
        set_attr(obj, 'ns_aux', float(aux))
        set_attr(obj, 'ns_lock', 0.0)
        set_attr(obj, 'ns_soft', 1.0 if soft else 0.0)
        obj['ns_slot'] = slot
        self.kinds.setdefault(slot, set()).add(fin)
        self.objs.append(obj)
        return obj

    def special(self, obj, mat_name):
        """Faces of an unbaked slot: their UVs (UV0 0..1 by the slot's rule) are kept."""
        set_attr(obj, 'ns_fin', 0.0)
        set_attr(obj, 'ns_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'ns_aux', 0.0)
        set_attr(obj, 'ns_lock', 1.0)
        if not obj.data.uv_layers:
            obj.data.uv_layers.new(name='UVMap')
        obj['ns_slot'] = mat_name
        self.objs.append(obj)
        return obj

    def panel(self, key, w, h, draw, glow=0.0, density=1.0):
        """A print panel w x h meters (drawn in millimeters)."""
        if self.atlas is None:
            self.atlas = Atlas(f'{self.prefix}_print', self.sizes.get(f'{self.prefix}_print', self.print_px))
            self.sizes.setdefault(f'{self.prefix}_print', self.print_px)
        self.atlas.add(key, w * 1000.0, h * 1000.0, draw, glow, density)

    def decal(self, key, origin, u=(1, 0, 0), v=(0, 0, 1), w=None, h=None, lift=0.0004, outline=None, nu=1, nv=1, disp=None, matrix=None,
              back=False, flip_u=False):
        """Panel `key` on the plane through origin spanned by u (the print's right) and v (its up), facing u x v (or
        away from it with back=True: the print then reads mirrored from that side, as gilding seen through glass)."""
        p = self.atlas.panels[key]
        w = p['w'] / 1000.0 if w is None else w
        h = p['h'] / 1000.0 if h is None else h
        u0, v0, u1, v1 = self.atlas.rect(key)
        if flip_u:
            u0, u1 = u1, u0
        U, V = Vector(u).normalized(), Vector(v).normalized()
        N = U.cross(V).normalized()
        if back:
            N = -N
        O = Vector(origin) + N * lift
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new('UVMap')

        def pos(s, t):
            d = disp(s, t) if disp else 0.0
            return O + U * ((s - 0.5) * w) + V * ((t - 0.5) * h) + N * d

        if outline:
            vs = [bm.verts.new(pos(s, t)) for s, t in outline]
            f = bm.faces.new(vs)
            for loop, (s, t) in zip(f.loops, outline):
                loop[uvl].uv = (u0 + (u1 - u0) * s, v0 + (v1 - v0) * t)
        else:
            grid = [[bm.verts.new(pos(i / nu, j / nv)) for i in range(nu + 1)] for j in range(nv + 1)]
            for j in range(nv):
                for i in range(nu):
                    f = bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
                    for loop, (a, b) in zip(f.loops, ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))):
                        loop[uvl].uv = (u0 + (u1 - u0) * a / nu, v0 + (v1 - v0) * b / nv)
        bm.normal_update()
        for f in bm.faces:
            if f.normal.dot(N) < 0:
                f.normal_flip()
        obj = core.mesh_object(f'decal_{key}', bm)
        if matrix is not None:
            obj.data.transform(matrix)
        set_attr(obj, 'ns_fin', 0.0)
        set_attr(obj, 'ns_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'ns_aux', 0.0)
        set_attr(obj, 'ns_lock', 1.0)
        obj['ns_slot'] = 'print'
        self.objs.append(obj)
        return obj

    def wrap_print(self, obj, key, uvfun):
        """Any mesh printed with panel `key`: uvfun(co) -> (s, t) in [0, 1]^2 of the panel (labels round a bottle)."""
        u0, v0, u1, v1 = self.atlas.rect(key)
        me = obj.data
        uv = me.uv_layers.get('UVMap') or me.uv_layers.new(name='UVMap')
        for poly in me.polygons:
            for li in poly.loop_indices:
                s, t = uvfun(me.vertices[me.loops[li].vertex_index].co)
                uv.data[li].uv = (u0 + (u1 - u0) * s, v0 + (v1 - v0) * t)
        set_attr(obj, 'ns_fin', 0.0)
        set_attr(obj, 'ns_col', (1.0, 1.0, 1.0))
        set_attr(obj, 'ns_aux', 0.0)
        set_attr(obj, 'ns_lock', 1.0)
        obj['ns_slot'] = 'print'
        self.objs.append(obj)
        return obj

    def finish(self, angle=35.0):
        name = self.name
        used = {o['ns_slot'] for o in self.objs}
        mats = {}
        order = []
        for s in self.slots:
            if s not in used:
                continue
            if s == 'print':
                mats[s] = print_material(self).m
            elif s.startswith('ns_'):
                mats[s] = UNB[s]
            else:
                mats[s] = body_material(self, s).m
                self.sizes.setdefault(f'{self.prefix}_{s}', 1024)
            order.append(s)
        missing = used - set(order)
        if missing:
            raise RuntimeError(f'{name}: slots used but not declared: {missing}')
        for o in self.objs:
            core.assign(o, mats[o['ns_slot']])
            if not o.data.uv_layers:
                o.data.uv_layers.new(name='UVMap')
            elif o.data.uv_layers[0].name != 'UVMap':
                o.data.uv_layers[0].name = 'UVMap'
        obj = core.join(self.objs, name) if len(self.objs) > 1 else self.objs[0]
        obj.name = name
        obj.data.name = name
        order_slots(obj, [mats[s] for s in order])
        finish_shading(obj, angle)
        lock = obj.data.attributes.get('ns_lock')
        vals = np.zeros(len(obj.data.polygons), dtype=np.float32)
        lock.data.foreach_get('value', vals)
        locked = set(np.nonzero(vals > 0.5)[0].tolist())
        tiled_idx = {}
        for slot, spec in self.tiled.items():
            if slot not in order:
                continue
            si = order.index(slot)
            tiled_idx[si] = slot
            me = obj.data
            uvl = me.uv_layers['UVMap']
            pu, pv = spec['period']
            ox, oy = spec['origin']
            z0, z1 = spec['z_range']
            first = None
            for poly in me.polygons:
                if poly.material_index != si:
                    continue
                locked.add(poly.index)
                first = first if first is not None else poly.index
                n = poly.normal
                for li in poly.loop_indices:
                    co = me.vertices[me.loops[li].vertex_index].co
                    if spec['mode'] == 'xy':
                        uvl.data[li].uv = ((co.x - ox) / pu, (co.y - oy) / pv)
                    else:
                        along = co.x if abs(n.y) >= abs(n.x) else co.y
                        uvl.data[li].uv = ((along - ox) / pu, (co.z - z0) / (z1 - z0))
            if first is not None:
                PROXIES.append(self._proxy(obj, slot, si, spec, first))
        obj['ns_tiled'] = {str(k): v for k, v in tiled_idx.items()}
        groups = [(lambda f: f.index in locked, 'keep', None, (0.0, 0.0, 1.0, 1.0))]
        for s in obj.material_slots:
            groups.append((core.material_is(obj, s.material.name), 'box', None, (0.0, 0.0, 1.0, 1.0)))
        groups.append((lambda f: True, 'box', None, (0.0, 0.0, 1.0, 1.0)))
        core.uv_layout(obj, groups)
        obj['ns_anchor'] = list(self.anchor)
        obj['ns_decl'] = [s.material.name for s in obj.material_slots]
        tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
        tl = f', tiled {sorted(self.tiled)}' if self.tiled else ''
        print(f'[{self.log}] {name}: {tris} triangles, slots {[s.material.name for s in obj.material_slots]}{tl}')
        return obj

    def _proxy(self, obj, slot, si, spec, face_index):
        """The square a tiled slot is baked on: the slot's material, the attributes of one of its faces."""
        pu, pv = spec['period']
        ox, oy = spec['origin']
        if spec['mode'] == 'xy':
            z = spec['z']
            pts = [(ox, oy, z), (ox + pu, oy, z), (ox + pu, oy + pv, z), (ox, oy + pv, z)]
            uvs = [(0, 0), (1, 0), (1, 1), (0, 1)]
            if spec['down']:
                pts, uvs = list(reversed(pts)), list(reversed(uvs))
        else:
            z0, z1 = spec['z_range']
            pts = [(ox, 0.0, z0), (ox + pu, 0.0, z0), (ox + pu, 0.0, z1), (ox, 0.0, z1)]
            uvs = [(0, 0), (1, 0), (1, 1), (0, 1)]
        p = quad(f'{obj.name}__{slot}', pts)
        p.name = f'{obj.name}__{slot}'
        uv = p.data.uv_layers.new(name='UVMap')
        for li, t in enumerate(uvs):
            uv.data[li].uv = t
        me = obj.data
        for an in ('ns_fin', 'ns_aux'):
            a = me.attributes.get(an)
            set_attr(p, an, a.data[face_index].value if a else 0.0)
        a = me.attributes.get('ns_col')
        set_attr(p, 'ns_col', tuple(a.data[face_index].vector) if a else (1.0, 1.0, 1.0))
        core.assign(p, obj.material_slots[si].material)
        p['ns_proxy_of'] = obj.name
        p['ns_proxy_slot'] = si
        return p


PROXIES = []


def finish_shading(obj, angle):
    """core.finish_hard_surface (smooth, sharp past `angle`, face-area weighted normals) on the hard parts only: faces
    added with soft=True (cloth, food) keep plain smooth vertex normals and no sharp edges, so a coat's folds or a
    pancake's rim don't facet."""
    me = obj.data
    a = me.attributes.get('ns_soft')
    vals = np.zeros(len(me.polygons), dtype=np.float32)
    if a is not None and a.domain == 'FACE':
        a.data.foreach_get('value', vals)
    if not (vals > 0.5).any():
        core.finish_hard_surface(obj, angle)
        return
    me.shade_smooth()
    try:
        me.set_sharp_from_angle(angle=math.radians(angle))
    except AttributeError:
        pass
    soft_v, hard_v = set(), set()
    for p in me.polygons:
        (soft_v if vals[p.index] > 0.5 else hard_v).update(p.vertices)
    only_soft = soft_v - hard_v
    sh = me.attributes.get('sharp_edge')
    if sh is not None:
        flags = np.zeros(len(me.edges), dtype=bool)
        sh.data.foreach_get('value', flags)
        for e in me.edges:
            if e.vertices[0] in only_soft and e.vertices[1] in only_soft:
                flags[e.index] = False
        sh.data.foreach_set('value', flags)
    vg = obj.vertex_groups.new(name='ns_hard')
    vg.add(sorted(hard_v), 1.0, 'REPLACE')
    mod = obj.modifiers.new('weighted', 'WEIGHTED_NORMAL')
    mod.mode = 'FACE_AREA'
    mod.keep_sharp = True
    mod.weight = 50
    mod.vertex_group = 'ns_hard'
    core.select_only([obj])
    bpy.ops.object.modifier_apply(modifier=mod.name)
    g = obj.vertex_groups.get('ns_hard')
    if g is not None:
        obj.vertex_groups.remove(g)
    obj.data.update()


def order_slots(obj, mats):
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


def glow(objs, name, anchor=(0.0, 0.0, 0.0), log='ns'):
    """A [glow:<family>] mesh: the parts that light up, one white emissive slot the game colors."""
    o = parts.glow_object(objs, name)
    o['ns_anchor'] = list(anchor)
    o['ns_unbaked'] = 1
    tris = sum(len(p.vertices) - 2 for p in o.data.polygons)
    print(f'[{log}] {name}: {tris} triangles, slots {[s.material.name for s in o.material_slots]} (glow)')
    return o


def unbaked_mesh(objs, name, mat_name, anchor=(0.0, 0.0, 0.0), log='ns'):
    """A mesh that is one unbaked slot as a whole (shop glass, mirrors), its UVs kept."""
    for o in objs:
        if not o.data.uv_layers:
            o.data.uv_layers.new(name='UVMap')
        core.assign(o, UNB[mat_name])
    obj = core.join(objs, name) if len(objs) > 1 else objs[0]
    obj.name = name
    obj.data.name = name
    obj.data.shade_smooth()
    try:
        obj.data.set_sharp_from_angle(angle=math.radians(30.0))
    except AttributeError:
        pass
    obj['ns_anchor'] = list(anchor)
    obj['ns_unbaked'] = 1
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    print(f'[{log}] {name}: {tris} triangles, slots {[s.material.name for s in obj.material_slots]}')
    return obj


# ------------------------------------------------------------------ bake split and assembly

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


def split_for_bake(objs):
    """The meshes to bake, each without its unbaked and tiled faces (split off, hidden, joined back in
    join_after_bake), plus the tiled slots' proxies. Wholly unbaked meshes (glass, glow) are hidden, not baked."""
    _SPLIT.clear()
    out = []
    for o in objs:
        if o.get('ns_unbaked'):
            o.hide_render = True
            continue
        tiled = {int(k) for k in (o.get('ns_tiled') or {}).keys()}
        idx = [i for i, s in enumerate(o.material_slots) if (s.material and s.material.name in UNB) or i in tiled]
        if idx:
            dup = o.copy()
            dup.data = o.data.copy()
            core.link(dup)
            dup.name = o.name + '__split'
            _delete_faces(o, lambda f: f.material_index in idx)
            _delete_faces(dup, lambda f: f.material_index not in idx)
            dup.hide_render = True
            kept = [i for i in range(len(o.material_slots)) if i not in idx]
            _prune_slots(o)
            _SPLIT[o.name] = (dup, kept, idx)
        out.append(o)
    for p in PROXIES:
        out.append(p)
    return out


def join_after_bake(all_objs, order):
    """Rejoins split meshes with their slots in the declared order (baked ones named M_<mesh>_<i>), gives tiled slots
    their proxy's baked material, moves SITE meshes from their local frame to the stage frame, and returns them in
    `order`. The proxies are hidden (not exported)."""
    proxy_mat = {(p['ns_proxy_of'], int(p['ns_proxy_slot'])): p.material_slots[0].material for p in PROXIES}
    for p in PROXIES:
        p.hide_render = True
    out = {}
    for o in all_objs:
        o.hide_render = False
        name = o.name
        sp = _SPLIT.get(name)
        if sp is not None:
            dup, kept, idx = sp
            decl = list(o.get('ns_decl') or [])
            final = [None] * (len(kept) + len(idx))
            for j, i in enumerate(kept):
                final[i] = o.material_slots[j].material
            for i in idx:
                final[i] = proxy_mat.get((name, i)) or dup.material_slots[i].material
            dup.hide_render = False
            o = core.join([o, dup], name)
            o.name = name
            o.data.name = name
            me = o.data
            fi = np.zeros(len(me.polygons), dtype=np.int32)
            me.polygons.foreach_get('material_index', fi)
            cur = [s.material for s in o.material_slots]
            # dup's faces still point at its pre-bake materials: map those by their declared index
            remap = []
            for mt in cur:
                if mt in final:
                    remap.append(final.index(mt))
                elif mt is not None and mt.name in decl:
                    remap.append(decl.index(mt.name))
                else:
                    remap.append(0)
            me.materials.clear()
            for mt in final:
                me.materials.append(mt)
            me.polygons.foreach_set('material_index', np.array(remap, dtype=np.int32)[fi])
            baked = [(i, mt) for i, mt in enumerate(final) if mt.name not in UNB]
            for i, mt in baked:
                mt.name = f'tmp_{name}_{i}'
            for i, mt in baked:
                mt.name = f'M_{name}_{i}'
        a = o.get('ns_anchor')
        if a is not None and any(abs(x) > 0 for x in a):
            o.data.transform(Matrix.Translation(Vector(a)))
            o['ns_anchor'] = [0.0, 0.0, 0.0]
        out[o.name] = o
    return [out[n] for n in order if n in out]


# ------------------------------------------------------------------ review rig

def rv_mat(name, hexc, rough=0.5, metal=0.0, emit=None, strength=0.0):
    m = core.Mat(name)
    m.set('Base Color', H(hexc))
    m.set('Roughness', rough)
    m.set('Metallic', metal)
    if emit is not None:
        m.set('Emission Color', H(emit))
        m.set('Emission Strength', strength)
    return m


def rv_light(name, kind, loc, energy, color=(1, 1, 1), size=0.1, size_y=None, target=None, spot=None, rot=None):
    data = bpy.data.lights.new(name, kind)
    data.energy = energy
    data.color = color
    if kind == 'AREA':
        data.shape = 'RECTANGLE' if size_y is not None else 'DISK'
        data.size = size
        if size_y is not None:
            data.size_y = size_y
    elif kind in ('POINT', 'SPOT'):
        data.shadow_soft_size = size
        if kind == 'SPOT' and spot:
            data.spot_size = math.radians(spot)
            data.spot_blend = 0.6
    elif kind == 'SUN':
        data.angle = math.radians(size)
    o = core.link(bpy.data.objects.new(name, data))
    o.location = loc
    if target is not None:
        d = Vector(target) - Vector(loc)
        o.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    elif rot is not None:
        o.rotation_euler = rot
    o.visible_camera = False
    return o


def rv_shot(cam, path, loc, target, lens, w=900, h=900, samples=96):
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


def rv_compose(name, paths, cols, log='ns'):
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
        out[y0:y0 + im.shape[0], x0:x0 + im.shape[1]] = im
    path = os.path.join(core.REVIEW_DIR, f'{name}.jpg')
    img = bpy.data.images.new(f'rv_{name}', W, Hh, alpha=False)
    img.pixels.foreach_set(out.ravel())
    img.filepath_raw = path
    img.file_format = 'JPEG'
    img.save(quality=90)
    bpy.data.images.remove(img)
    print(f'[{log}] review sheet {os.path.relpath(path, core.ROOT)}')
    return path


def glow_tint(obj, hexc, strength):
    """Review only: color a glow mesh as the game will."""
    for s in obj.material_slots:
        mt = s.material
        if mt is None or not mt.use_nodes:
            continue
        b = next((n for n in mt.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
        if b is not None:
            b.inputs['Emission Color'].default_value = H(hexc)
            b.inputs['Base Color'].default_value = H(hexc)
            b.inputs['Emission Strength'].default_value = strength


def instance(obj, loc, yaw_deg, name=None):
    """Review only: a placed copy of a PROP (stage placement: Blender location, yaw about Z)."""
    d = obj.copy()
    core.link(d)
    d.name = name or (obj.name + '_rv')
    d.location = loc
    d.rotation_mode = 'XYZ'
    d.rotation_euler = (0.0, 0.0, math.radians(yaw_deg))
    return d


def prop_yaw(stage_yaw):
    """The Blender yaw that puts a PROP (front -X) facing stage yaw F in the SITE review: the SITE frame is the stage
    turned 180, so the PROP's yaw F + 180 in the stage is yaw -F in Blender (as the X mirror)."""
    return -stage_yaw


# =====================================================================================================================
# THE BARBERSHOP
# =====================================================================================================================

LOG = 'ns_barber'
ANCHOR = (0.0, 33.65, 0.0)        # Blender: the opening's middle on the front plane at the floor (stage (0, 3365, 0))
OPEN_Y = (-2.53, 2.53)            # the opening (local y), Z 0..3.35
ROOM_X = (0.35, 9.00)             # the interior (local x = Blender x)
ROOM_Y = (-2.40, 2.45)
CEIL = 3.30
DOOR_Y = (-2.33, -1.40)           # the leaf's extent (hinge at the south end)
DOOR_X = 0.645                    # the hinge axis and the leaf's inside face (the leaf is 45 mm, its outside at 0.60)
DOOR_H = 2.30
GLASS_X = 0.11                    # the display glass plane
SASH = 0x1d3a2c                   # the storefront's dark green enamel
CHAIR_X = (2.60, 4.60)            # the chairs' pedestal axes (local x)
CHAIR_Y = 0.95
WAIT_X = (1.50, 2.05, 3.10, 3.65)
WAIT_Y = -2.05
GRILLE_X = -0.09                  # the grille curtain's plane (in front of the storefront, between the piers)
GRILLE_TOP = 3.07

NAMES = {
    'front': ['SM_NS_Barber_Front', 'SM_NS_Barber_Front_Glass', 'SM_NS_Barber_Front_GlowNeon'],
    'door': ['SM_NS_Barber_Door', 'SM_NS_DoorBell'],
    'grille': ['SM_NS_Barber_Grille'],
    'pole': ['SM_NS_Barber_Pole', 'SM_NS_Barber_Pole_GlowWarm'],
    'shell': ['SM_NS_Barber_Shell'],
    'chair': ['SM_NS_Barber_Chair'],
    'station': ['SM_NS_Barber_Station'],
    'mirror': ['SM_NS_Barber_Mirror'],
    'waiting': ['SM_NS_Barber_Waiting'],
    'wall': ['SM_NS_Barber_Wall'],
    'sink': ['SM_NS_Barber_Sink'],
    'cape': ['SM_NS_Barber_Cape'],
    'lights': ['SM_NS_Barber_Lights', 'SM_NS_Barber_Lights_GlowWarm', 'SM_NS_Barber_Lights_GlowCool'],
}
_ONLY = [s.strip() for s in os.environ.get('NSB_ONLY', '').split(',') if s.strip()]
WHICH = [k for k in NAMES if not _ONLY or k in _ONLY]
MESHES = [n for k in WHICH for n in NAMES[k]]
DOUBLE_SIDED = False
TEXTURE_SIZE = 1024
TEXTURE_SIZES = {}
AO_DISTANCE = 0.04
REVIEW_SCREEN_LIGHT = False
REVIEW_VIEWS = None
ALL = []


def font(face):
    return bpy.data.fonts.load(os.path.join(core.FONTS, f'Roboto-{face}.ttf'), check_existing=True)


def text_mesh(name, text, size, M, face='Black', align='CENTER', offset=0.0, tracking=1.0, squeeze=1.0, res=3, shift=(0.0, 0.0)):
    """Lettering as a flat mesh (sharp at any distance, no alpha): laid out in its own XY (baseline at y 0, centered
    or aligned on x 0), then placed by M (X -> the print's right, Y -> its up, Z -> its face normal)."""
    cu = bpy.data.curves.new(name, 'FONT')
    cu.body = text
    cu.font = font(face)
    cu.size = size
    cu.align_x = align
    cu.space_character = tracking
    cu.offset = offset
    cu.resolution_u = res
    ob = core.link(bpy.data.objects.new(name, cu))
    core.select_only([ob])
    bpy.ops.object.convert(target='MESH')
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    me = obj.data
    for v in me.vertices:
        v.co.x = v.co.x * squeeze + shift[0]
        v.co.y += shift[1]
        v.co.z = 0.0
    weld(obj, 1e-7)
    me.transform(M)
    me.update()
    return obj


def text_width(text, size, face='Black', tracking=1.0):
    cu = bpy.data.curves.new('w', 'FONT')
    cu.body = text
    cu.font = font(face)
    cu.size = size
    cu.space_character = tracking
    ob = core.link(bpy.data.objects.new('w', cu))
    bpy.context.view_layer.update()
    w = ob.dimensions.x
    bpy.data.objects.remove(ob)
    bpy.data.curves.remove(cu)
    return w


def kit(prefix, name, slots, anchor=ANCHOR, **kw):
    return Kit(prefix, name, slots, sizes=TEXTURE_SIZES, anchor=anchor, log=LOG, **kw)


def reverse(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()
    return obj


def street_letters(k, text, size, origin, u, v, gold=True, face='Black', tracking=1.06, squeeze=1.0, shade=0x1c140a, line=0x0c0b0a,
                   back=True, back_col=0x1a1612, face_col=None):
    """Gilded lettering on glass (or painted lettering on a board): the face (gold leaf), a keyline and a drop shade
    behind it, and with back=True the back of the gilding as seen through the glass from the other side (its dark
    backing paint, reading mirrored)."""
    U, V = Vector(u).normalized(), Vector(v).normalized()
    N = U.cross(V).normalized()
    M = frame_matrix(U, V, Vector(origin))
    lift = N * 0.0002
    face_o = text_mesh('gild', text, size, Matrix.Translation(lift * 3) @ M, face=face, tracking=tracking, squeeze=squeeze)
    key = text_mesh('key', text, size, Matrix.Translation(lift * 2) @ M, face=face, tracking=tracking, squeeze=squeeze, offset=size * 0.035)
    sh = text_mesh('shade', text, size, Matrix.Translation(lift) @ M, face=face, tracking=tracking, squeeze=squeeze, offset=size * 0.035,
                   shift=(size * 0.045, -size * 0.05))
    if gold:
        k.add(face_o, 'gilt', 0xc9a24a)
    else:
        k.add(face_o, 'enamel', face_col if face_col is not None else 0xe9e2cf)
    k.add(key, 'enamel', line)
    k.add(sh, 'enamel', shade)
    if back:
        bk = text_mesh('gild_back', text, size, Matrix.Translation(-lift) @ M, face=face, tracking=tracking, squeeze=squeeze, offset=size * 0.035)
        k.add(reverse(bk), 'enamel', back_col)


@finish_kind('gilt', metal=True)
def f_gilt(c):
    """Gold leaf on glass: burnished, a faint engine-turned matte band, a few cracks and losses."""
    m = c.m
    turn = ramp(m, noise(m, scaled(m, c.sep, 300.0, 300.0, 300.0), detail=1.0), 0.45, 0.6)
    loss = ramp(m, noise(m, c.P, scale=180.0, detail=3.0), 0.78, 0.81)
    col = m.mix(mul(m, turn, 0.3), H(0xe8c36a), H(0xcfa047))
    col = m.mix(loss, col, H(0x16120e))
    return col, add(m, 0.26, mul(m, turn, 0.14), mul(m, loss, 0.4)), mix_float(m, loss, 1.0, 0.0), 0.0


@finish_kind('galv', metal=True)
def f_galv(c):
    """Galvanized steel outdoors: dull zinc spangle, white rust, brown rust where the zinc has gone, grime low."""
    m = c.m
    sp = noise(m, c.P, scale=60.0, detail=1.0, distortion=1.5)
    pat = noise(m, c.P, scale=5.0, detail=4.0)
    rust = ramp(m, noise(m, c.P, scale=14.0, detail=5.0), 0.66, 0.74)
    white = ramp(m, noise(m, c.P, scale=30.0, detail=3.0), 0.6, 0.7)
    col = m.mix(mul(m, sp, 0.25), H(0x9ea3a6), H(0xc0c4c6))
    col = m.mix(mul(m, pat, 0.5), col, H(0x74787a))
    col = m.mix(mul(m, white, 0.5), col, H(0xd2d2cc))
    col = m.mix(rust, col, H(0x6a3a1e))
    return col, add(m, 0.5, mul(m, sp, 0.12), mul(m, rust, 0.3), mul(m, white, 0.2)), mix_float(m, add(m, rust, mul(m, white, 0.6)), 0.85, 0.0), mul(m, rust, 0.05)


def neon_open_paths(h):
    """OPEN in single-stroke neon letters h tall: (lit paths, jumper paths, total width), each path a list of (s, t)
    points in meters, s to the print's right (centered), t up from the baseline."""
    w = h * 0.64
    gap = h * 0.24

    def ell(xc, yc, rx, ry, n, a0, a1):
        return [(xc + rx * math.cos(a0 + (a1 - a0) * i / n), yc + ry * math.sin(a0 + (a1 - a0) * i / n)) for i in range(n + 1)]

    lit, jump = [], []
    o = ell(w / 2, h / 2, w / 2, h / 2, 40, -math.pi / 2 + 0.2, 1.5 * math.pi - 0.05)
    lit.append(o)
    x1 = w + gap
    bowl = [(x1 + w * 0.42 + w * 0.5 * math.cos(a), h * 0.745 + h * 0.255 * math.sin(a)) for a in [math.pi / 2 - math.pi * i / 18 for i in range(19)]]
    p = [(x1, 0.0), (x1, h), (x1 + w * 0.42, h)] + bowl + [(x1 + w * 0.42, h * 0.49), (x1, h * 0.49)]
    lit.append(p)
    jump.append([o[0], (o[0][0] + h * 0.05, -h * 0.06), (x1 - h * 0.04, -h * 0.06), (x1, 0.0)])
    x2 = x1 + w + gap * 0.9
    e = [(x2 + w * 0.86, h), (x2, h), (x2, 0.0), (x2 + w * 0.86, 0.0)]
    lit.append(e)
    lit.append([(x2, h * 0.5), (x2 + w * 0.7, h * 0.5)])
    jump.append([(x1, h * 0.49), (x1 + w * 0.2, h * 1.08), (x2 + w * 0.86, h * 1.08), (x2 + w * 0.86, h)])
    x3 = x2 + w * 0.86 + gap
    lit.append([(x3, 0.0), (x3, h), (x3 + w * 0.9, 0.0), (x3 + w * 0.9, h)])
    jump.append([(x2 + w * 0.86, 0.0), (x2 + w * 0.86 + gap * 0.5, -h * 0.07), (x3, 0.0)])
    total = x3 + w * 0.9
    return ([[(s - total / 2, t) for s, t in pth] for pth in lit], [[(s - total / 2, t) for s, t in pth] for pth in jump], total)


def prism_y(name, outline, y0, y1):
    """A prism along Y from an outline of (x, z) points (moldings, sills, rails)."""
    bm = bmesh.new()
    a = [bm.verts.new((x, y0, z)) for x, z in outline]
    b = [bm.verts.new((x, y1, z)) for x, z in outline]
    bm.faces.new(a)
    bm.faces.new(list(reversed(b)))
    n = len(outline)
    for i in range(n):
        bm.faces.new((a[i], a[(i + 1) % n], b[(i + 1) % n], b[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return core.mesh_object(name, bm)


def molding(name, path2, profile, z0=0.0, closed=False):
    """A molding swept along a horizontal 2D path at height z0: a closed profile of (out, up) points, out = to the
    left of the path's direction (walk a room counter-clockwise and the room is on the left). Ends are capped."""
    pts = [Vector((x, y, 0.0)) for x, y in path2]
    n = len(pts)
    bm = bmesh.new()
    rings = []
    for i in range(n):
        if not closed and i == 0:
            t = (pts[1] - pts[0]).normalized()
            left, k = Vector((-t.y, t.x, 0.0)), 1.0
        elif not closed and i == n - 1:
            t = (pts[-1] - pts[-2]).normalized()
            left, k = Vector((-t.y, t.x, 0.0)), 1.0
        else:
            t0 = (pts[i] - pts[i - 1]).normalized()
            t1 = (pts[(i + 1) % n] - pts[i]).normalized()
            l0 = Vector((-t0.y, t0.x, 0.0))
            l1 = Vector((-t1.y, t1.x, 0.0))
            left = (l0 + l1).normalized()
            k = 1.0 / max(0.3, left.dot(l0))
        rings.append([bm.verts.new(pts[i] + left * (o * k) + Vector((0, 0, z0 + u))) for o, u in profile])
    m = len(profile)
    for i in range(n if closed else n - 1):
        r0, r1 = rings[i], rings[(i + 1) % n]
        for j in range(m):
            bm.faces.new((r0[j], r0[(j + 1) % m], r1[(j + 1) % m], r1[j]))
    if not closed:
        bm.faces.new(rings[0])
        bm.faces.new(list(reversed(rings[-1])))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return core.mesh_object(name, bm)


@finish_kind('leaf')
def f_leaf(c):
    """A waxy house-plant leaf (a snake plant): dark green with paler cross-bands, a yellow margin."""
    m = c.m
    band = ramp(m, noise(m, scaled(m, c.sep, 30.0, 30.0, 120.0), detail=3.0, distortion=1.5), 0.5, 0.62)
    col = m.mix(mul(m, band, 0.6), c.col, H(0x7c9a62))
    col = m.mix(mul(m, is_k(m, c.aux, 1), ramp(m, c.edges( distance=0.004, samples=8, breakup_scale=40.0), 0.2, 0.5)), col, H(0xc9b64a))
    dust = ramp(m, noise(m, c.P, scale=40.0, detail=3.0), 0.5, 0.8)
    return col, add(m, 0.32, mul(m, dust, 0.3)), 0.0, mul(m, band, 0.05)


@finish_kind('soil')
def f_soil(c):
    m = c.m
    v = voronoi(m, c.P, 300.0, 'F1', 'Distance')
    col = m.mix(ramp(m, v, 0.0, 0.8), H(0x2a1d14), H(0x4a3424))
    return col, 0.95, 0.0, mul(m, v, 0.6)


# ------------------------------------------------------------------ the barber's storefront

def draw_license(s, w, h):
    s.rect(0, 0, w, h, 0xf1ede2, rough=0.6)
    s.rect(2, h - 14, w - 4, 12, 0x2b4a7a, rough=0.5)
    s.mid('CITY BUSINESS LICENSE', w / 2, h - 8, 7, w - 10, 0xf1ede2, 'Bold')
    s.mid('BARBER SHOP · CLASS B', w / 2, h - 26, 6, w - 10, 0x222222, 'Bold')
    s.mid('FIFTH ST. BARBERS', w / 2, h - 38, 6, w - 10, 0x222222, 'Medium')
    s.mid('1818 FIFTH ST', w / 2, h - 47, 5, w - 10, 0x222222, 'Regular')
    s.circle(w - 14, 14, 10, 0xc9a24a, metal=0.6, rough=0.3)
    s.mid('2026', w / 2 - 6, 12, 9, 40, 0xa3221c, 'Black')


def draw_cash(s, w, h):
    """A hand-lettered card propped in the window."""
    s.rect(0, 0, w, h, 0xf3efe4, rough=0.8)
    s.mid('CASH', w / 2, h * 0.66, h * 0.32, w - 12, 0x1a1a1a, 'Black')
    s.mid('ONLY', w / 2, h * 0.3, h * 0.26, w - 12, 0xa3221c, 'Black')


def draw_tin(label, ground, ink):
    def draw(s, w, h):
        s.circle(w / 2, h / 2, w / 2, ground, rough=0.35, metal=0.3)
        s.ring(w / 2, h / 2, w * 0.40, w * 0.46, ink, rough=0.35)
        s.mid(label, w / 2, h * 0.56, h * 0.2, w * 0.7, ink, 'Black')
        s.mid('POMADE', w / 2, h * 0.36, h * 0.08, w * 0.5, ink, 'Bold', tracking=1.4)
    return draw


def neon_tube(name, pts2, M, r=0.006, n=8):
    pts = [M @ Vector((s, t, 0.0)) for s, t in pts2]
    pts = bend([tuple(p) for p in pts], r * 2.2, 3, min_turn=25.0)
    return tube(name, pts, r, n)


def pane_x(x, y0, y1, z0, z1):
    """A glass pane in the x plane, UV 0..1 as seen from -X (u toward -y)."""
    o = quad('pane', [(x, y1, z0), (x, y0, z0), (x, y0, z1), (x, y1, z1)])
    uv = o.data.uv_layers.new(name='UVMap')
    for li, t in enumerate(((0, 0), (1, 0), (1, 1), (0, 1))):
        uv.data[li].uv = t
    return o


def pane_y(y, x0, x1, z0, z1):
    o = quad('pane', [(x0, y, z0), (x1, y, z0), (x1, y, z1), (x0, y, z1)])
    uv = o.data.uv_layers.new(name='UVMap')
    for li, t in enumerate(((0, 0), (1, 0), (1, 1), (0, 1))):
        uv.data[li].uv = t
    return o


def snake_plant(k, px, py, z0, rnd, n=9, scale=1.0):
    for i in range(n):
        a = rnd.uniform(0, 2 * math.pi)
        L = rnd.uniform(0.35, 0.62) * scale
        lean = rnd.uniform(0.05, 0.22) * scale
        base = Vector((px + 0.03 * scale * math.cos(a), py + 0.03 * scale * math.sin(a), z0))
        tip = base + Vector((lean * math.cos(a), lean * math.sin(a), L))
        d = Vector((-math.sin(a), math.cos(a), 0.0))
        nrm = Vector((math.cos(a), math.sin(a), 0.0))
        tw = rnd.uniform(-0.6, 0.6)
        rings = []
        for j in range(9):
            t = j / 8
            c = base.lerp(tip, t) + nrm * (0.03 * scale * math.sin(math.pi * t))
            wdt = 0.032 * scale * math.sin(math.pi * min(0.98, 0.15 + t * 0.85)) * (1.0 - 0.8 * t ** 3) + 0.001
            th = 0.006 * scale * (1 - t) + 0.0015
            ang = tw * t
            dd = d * math.cos(ang) + nrm * math.sin(ang)
            nn = nrm * math.cos(ang) - d * math.sin(ang)
            rings.append([c + dd * (wdt * math.cos(q)) + nn * (th * math.sin(q) * (1.0 + 2.0 * abs(math.cos(q)))) for q in
                          [2 * math.pi * s / 8 for s in range(8)]])
        lf = core.loft('leaf', rings)
        lf.data.shade_smooth()
        k.add(lf, 'leaf', 0x2f4a2a, aux=1)


def front():
    k = kit('nsbf', 'SM_NS_Barber_Front', ('frame', 'tile', 'print'), dust=0.15, low=0.25)
    TEXTURE_SIZES.update({'nsbf_frame': 2048, 'nsbf_tile': 1024, 'nsbf_print': 1024})
    k.panel('license', 0.11, 0.075, draw_license, density=2.0)
    k.panel('cash', 0.16, 0.11, draw_cash, density=1.5)
    for i, (lab, gnd, ink) in enumerate((('SLICK', 0xc9302a, 0xf2e6c8), ('ROYALE', 0x1f3b6a, 0xe2c27a), ('GRIP', 0x1c1c1c, 0xd8b45a))):
        k.panel(f'tin{i}', 0.08, 0.08, draw_tin(lab, gnd, ink), density=1.6)
    k.atlas.render()
    sy0, sy1 = OPEN_Y
    T = lambda o, col, aux=1: k.add(o, 'mosaic', col, 'tile', aux=aux)
    E = lambda o: k.add(o, 'enamel', SASH)
    # --- the bulkhead: bottle-green glazed squares over a black base course, a bullnose cap
    T(bx('bulk', 0.07, 0.42, -1.40, 2.50, 0.10, 0.48), 0x1f4a3a)
    T(bx('bulk_base', 0.065, 0.42, -1.40, 2.50, 0.0, 0.10), 0x141414)
    T(parts.rod('bull', (0.09, -1.40, 0.48), (0.09, 2.50, 0.48), 0.022, 16), 0x1f4a3a)
    # the return into the recess (its tiled face toward -y)
    T(bx('ret_bulk', 0.07, 0.62, -1.42, -1.30, 0.10, 0.48), 0x1f4a3a)
    T(bx('ret_base', 0.065, 0.62, -1.425, -1.30, 0.0, 0.10), 0x141414)
    T(parts.rod('ret_bull', (0.07, -1.40, 0.48), (0.62, -1.40, 0.48), 0.022, 16), 0x1f4a3a)
    # the recess floor: white hex mosaic with a black border, a brass threshold at the door
    T(bx('recess_floor', 0.0, 0.60, -2.35, -1.42, -0.004, 0.006), 0xe6e2d6, 0)
    for (a, b, c_, d) in ((0.0, 0.60, -2.35, -2.30), (0.0, 0.60, -1.47, -1.42), (0.0, 0.05, -2.30, -1.47)):
        T(bx('recess_border', a, b, c_, d, -0.004, 0.0065), 0x161616, 0)
    k.add(bx('threshold', 0.585, 0.665, -2.35, -1.40, 0.0, 0.012, 0.004), 'brass')
    # --- the sash (painted wood) and the sill
    E(prism_y('sill', [(0.035, 0.498), (0.30, 0.498), (0.30, 0.565), (0.075, 0.565), (0.035, 0.548)], -1.32, 2.43))
    E(bx('jamb_n', 0.035, 0.17, 2.43, 2.53, 0.50, 3.35, 0.006))
    E(bx('mullion', 0.06, 0.165, 0.52, 0.60, 0.56, 2.40, 0.005))
    E(bx('corner_post', 0.035, 0.17, -1.43, -1.31, 0.0, 3.35, 0.006))
    E(bx('rail_lo', 0.07, 0.16, -1.31, 2.43, 0.56, 0.64, 0.004))
    E(bx('head', 0.025, 0.175, sy0, sy1, 2.38, 2.50, 0.008))
    E(bx('head_cap', 0.015, 0.06, sy0, sy1, 2.47, 2.505, 0.006))
    E(bx('trans_top', 0.055, 0.165, sy0, sy1, 3.27, 3.35, 0.005))
    E(bx('trans_mull', 0.06, 0.165, -1.43, -1.33, 2.50, 3.27, 0.005))
    E(bx('trans_jamb_s', 0.06, 0.165, sy0, -2.35, 2.50, 3.27, 0.005))
    for (y0, y1) in ((2.445, 2.515), (-1.415, -1.325)):
        E(bx('post_panel', 0.028, 0.036, y0, y1, 0.70, 2.30, 0.003))
    for (y0, y1) in ((-1.31, 0.52), (0.60, 2.43)):
        k.add(bx('bead_lo', 0.098, 0.106, y0, y1, 0.64, 0.655, 0.002), 'brass')
        k.add(bx('bead_hi', 0.098, 0.106, y0, y1, 2.365, 2.38, 0.002), 'brass')
        k.add(bx('bead_s', 0.098, 0.106, y0, y0 + 0.012, 0.64, 2.38, 0.002), 'brass')
        k.add(bx('bead_n', 0.098, 0.106, y1 - 0.012, y1, 0.64, 2.38, 0.002), 'brass')
    # the return's sash, the door's frame, the south casing, the recess soffit
    E(bx('ret_rail', 0.16, 0.60, -1.42, -1.33, 0.48, 0.56, 0.004))
    E(bx('ret_head', 0.16, 0.66, -1.43, -1.31, 2.30, 2.40, 0.004))
    E(bx('door_jamb_n', 0.56, 0.68, -1.43, -1.38, 0.0, 2.40, 0.004))
    E(bx('door_head', 0.56, 0.68, -2.36, -1.38, 2.30, 2.40, 0.004))
    E(bx('door_stop_n', 0.575, 0.60, -1.41, -1.385, 0.012, 2.30, 0.002))
    E(bx('door_stop_h', 0.575, 0.60, -2.35, -1.385, 2.275, 2.30, 0.002))
    E(bx('casing_s', 0.0, 0.68, sy0, -2.35, 0.10, 2.40, 0.006))
    T(bx('casing_base', -0.004, 0.68, sy0 - 0.004, -2.346, 0.0, 0.10), 0x141414)
    E(bx('door_stop_s', 0.575, 0.60, -2.35, -2.325, 0.012, 2.30, 0.002))
    E(bx('soffit', 0.0, 0.68, -2.35, -1.31, 2.40, 2.43))
    for z in (0.25, 1.15, 2.05):
        k.add(parts.rod('hinge', (DOOR_X + 0.004, DOOR_Y[0] - 0.006, z - 0.05), (DOOR_X + 0.004, DOOR_Y[0] - 0.006, z + 0.05), 0.0075, 12), 'brass')
    k.add(bx('strike', 0.60, 0.645, -1.385, -1.38, 0.98, 1.10, 0.001), 'brass')
    # the bell's bracket on the door head (inside): SM_NS_DoorBell hangs from its arm's end (BELL_ROOT)
    k.add(bx('bell_plate', 0.68, 0.686, -1.66, -1.54, 2.31, 2.39, 0.002), 'brass')
    k.add(tube('bell_arm', bend([(0.684, -1.60, 2.37), (0.72, -1.60, 2.37), (0.72, -1.60, 2.40)], 0.01, 3), 0.004, 8), 'brass')
    # --- the window ledge inside, on the bulkhead's back, its face paneled
    k.add(bx('ledge', 0.15, 0.44, -1.31, 2.43, 0.62, 0.645, 0.006), 'wood', 0x5a3420, aux=1)
    E(bx('ledge_face', 0.42, 0.435, -1.30, 2.42, 0.08, 0.62, 0.003))
    for y0 in (-1.2, -0.2, 0.8, 1.8):
        E(bx('ledge_panel', 0.435, 0.442, y0 + 0.06, y0 + 0.86, 0.16, 0.54, 0.004))
    E(bx('ledge_base', 0.42, 0.45, -1.30, 2.42, 0.0, 0.08))
    # on the ledge: a snake plant in a terracotta pot, pomade tins, the CASH ONLY card, a tonic bottle
    px, py = 0.30, 2.05
    k.add(lathe_solid('pot', [(0.0, 0.645), (0.075, 0.645), (0.095, 0.79), (0.105, 0.79), (0.105, 0.82), (0.0, 0.82)], Matrix.Translation((px, py, 0.0)), 32),
          'ceramic', 0xa65b3c)
    k.add(lathe_solid('soil', [(0.0, 0.80), (0.093, 0.80), (0.093, 0.806), (0.0, 0.806)], Matrix.Translation((px, py, 0.0)), 24), 'soil')
    snake_plant(k, px, py, 0.80, random.Random(1818))
    for i, (ty, tz) in enumerate(((1.30, 0.645), (1.40, 0.645), (1.35, 0.672))):
        cx = 0.26
        k.add(lathe_solid('tin', [(0.0, 0.0), (0.038, 0.0), (0.039, 0.003), (0.039, 0.024), (0.0, 0.024)], Matrix.Translation((cx, ty, tz)), 32), 'chrome')
        k.add(lathe_solid('tin_lid', [(0.0, 0.0), (0.0405, 0.0), (0.0405, 0.008), (0.0, 0.008)], Matrix.Translation((cx, ty, tz + 0.019)), 32), 'enamel',
              (0xc9302a, 0x1f3b6a, 0x1c1c1c)[i])
        k.decal(f'tin{i}', (cx, ty, tz + 0.027), u=(0, -1, 0), v=(1, 0, 0), w=0.074, h=0.074, lift=0.0003,
                outline=[(0.5 + 0.5 * math.cos(2 * math.pi * q / 24), 0.5 + 0.5 * math.sin(2 * math.pi * q / 24)) for q in range(24)])
    ang = math.radians(12.0)
    k.decal('cash', (0.17 + 0.055 * math.sin(ang), -0.90, 0.645 + 0.055 * math.cos(ang)), u=(0, -1, 0), v=(-math.sin(ang), 0, math.cos(ang)), lift=0.0005)
    k.add(lathe_solid('tonic', [(0.0, 0.0), (0.032, 0.0), (0.034, 0.01), (0.034, 0.15), (0.016, 0.19), (0.012, 0.215), (0.0, 0.215)],
                      Matrix.Translation((0.27, 1.62, 0.645)), 32), 'glassdark', 0x5a2e10)
    k.add(lathe_solid('tonic_cap', [(0.0, 0.0), (0.014, 0.0), (0.014, 0.03), (0.0, 0.03)], Matrix.Translation((0.27, 1.62, 0.86)), 24), 'chrome')
    k.decal('license', (GLASS_X + 0.0025, 0.38, 0.74), u=(0, -1, 0), v=(0, 0, 1), lift=0.0)
    # --- gold leaf: FIFTH ST. BARBERS / EST. 1995 over the display, WALK-INS / WELCOME over the door
    gx = GLASS_X + 0.0015
    ty0, ty1 = -1.33, 2.43
    big = 'FIFTH ST. BARBERS'
    size = min(0.30, (ty1 - ty0) * 0.86 / text_width(big, 1.0, 'Black', 1.06))
    street_letters(k, big, size, (gx, (ty0 + ty1) / 2, 2.80), (0, -1, 0), (0, 0, 1))
    street_letters(k, 'EST. 1995', 0.095, (gx, (ty0 + ty1) / 2, 2.585), (0, -1, 0), (0, 0, 1), tracking=1.25, face='Bold')
    for sgn in (-1, 1):
        cy = (ty0 + ty1) / 2 + sgn * 0.62
        k.add(bx('flourish', gx + 0.0006, gx + 0.0008, cy - 0.36, cy + 0.36, 2.615, 2.627), 'gilt')
        k.add(bx('flourish_key', gx + 0.0003, gx + 0.0005, cy - 0.365, cy + 0.365, 2.611, 2.631), 'enamel', 0x0c0b0a)
    dyc = (DOOR_Y[0] + DOOR_Y[1]) / 2
    street_letters(k, 'WALK-INS', 0.15, (gx, dyc, 2.93), (0, -1, 0), (0, 0, 1), tracking=1.08)
    street_letters(k, 'WELCOME', 0.15, (gx, dyc, 2.70), (0, -1, 0), (0, 0, 1), tracking=1.08)
    # --- the neon OPEN in the south light: tubes (glow mesh), black jumpers, electrodes, its frame and chains
    lit, jump, total = neon_open_paths(0.17)
    NX, NY, NZ = 0.15, -0.40, 1.46
    M = frame_matrix((0, -1, 0), (0, 0, 1), (NX, NY, NZ))
    glow_parts = [neon_tube('neon', p, M) for p in lit]
    for p in jump:
        k.add(neon_tube('jumper', p, M, r=0.005), 'enamel', 0x0d0d0d)
    for p in lit:
        for end in (p[0], p[-1]):
            e0 = M @ Vector((end[0], end[1], 0.0))
            k.add(parts.rod('electrode', tuple(e0), tuple(e0 + Vector((0.035, 0, 0))), 0.008, 12), 'glassdark', 0x1a1a1a)
            k.add(parts.rod('electrode_cap', tuple(e0 + Vector((0.035, 0, 0))), tuple(e0 + Vector((0.05, 0, 0))), 0.009, 12), 'rubber', 0x111111)
    fw, fh = total + 0.10, 0.17 + 0.12
    fx = NX + 0.05
    for (a, b, c_, d) in ((-fw / 2, fw / 2, -0.07, -0.058), (-fw / 2, fw / 2, fh - 0.082, fh - 0.07), (-fw / 2, -fw / 2 + 0.012, -0.07, fh - 0.07),
                          (fw / 2 - 0.012, fw / 2, -0.07, fh - 0.07)):
        k.add(bx('neon_frame', fx - 0.006, fx + 0.006, NY - b, NY - a, NZ + c_, NZ + d), 'enamel', 0x101010)
    for s_ in (-0.25, -0.08, 0.1, 0.24):
        k.add(bx('neon_wire', fx - 0.002, fx + 0.002, NY - s_ - 0.002, NY - s_ + 0.002, NZ - 0.064, NZ + fh - 0.076), 'enamel', 0x101010)
    for p in lit:
        for q in (p[len(p) // 3], p[2 * len(p) // 3]):
            e0 = M @ Vector((q[0], q[1], 0.0))
            k.add(parts.rod('standoff', tuple(e0 + Vector((0.006, 0, 0))), (fx, e0.y, e0.z), 0.004, 8), 'plastic', 0xd8d8d0)
    k.add(bx('transformer', fx - 0.03, fx + 0.03, NY - 0.06, NY + 0.06, NZ - 0.14, NZ - 0.07, 0.006), 'enamel', 0x202020)
    k.add(tube('neon_cord', bend([(fx, NY + 0.05, NZ - 0.14), (fx + 0.02, NY + 0.05, NZ - 0.5), (0.30, NY + 0.02, 0.648)], 0.08, 6), 0.003, 6), 'rubber', 0x111111)
    for s_ in (-fw / 2 + 0.03, fw / 2 - 0.03):
        z = NZ + fh - 0.07
        while z < 2.37:
            k.add(parts.rod('chain', (fx, NY - s_, z), (fx, NY - s_, min(z + 0.018, 2.38)), 0.0018, 6), 'chrome')
            z += 0.02
    # --- the grille's hood and guides, in front of the opening between the piers
    k.add(bx('hood', -0.24, -0.008, -2.50, 2.50, GRILLE_TOP, 3.35, 0.01), 'enamel', 0x24282a)
    k.add(bx('hood_lip', -0.245, -0.20, -2.50, 2.50, GRILLE_TOP - 0.012, GRILLE_TOP + 0.02, 0.004), 'enamel', 0x24282a)
    for s_ in (-1, 1):
        y_ = s_ * 2.50
        for (a, b) in ((-0.125, -0.115), (-0.065, -0.055)):
            k.add(bx('guide', a, b, y_ - 0.03, y_ + 0.03, 0.0, GRILLE_TOP), 'iron')
        k.add(bx('guide_back', -0.125, -0.055, y_ + s_ * 0.02, y_ + s_ * 0.03, 0.0, GRILLE_TOP), 'iron')
        k.add(bx('guide_shoe', -0.13, -0.05, y_ - 0.035, y_ + 0.035, 0.0, 0.02, 0.003), 'iron')
    k.grime = [((0.6, -1.85, 1.05), 0.2, 0.6), ((0.62, -1.45, 1.0), 0.15, 0.5)]
    obj = k.finish(40.0)
    g = [pane_x(GLASS_X, y0, y1, z0, z1) for (y0, y1, z0, z1) in ((-1.31, 0.52, 0.64, 2.38), (0.60, 2.43, 0.64, 2.38), (-1.33, 2.43, 2.50, 3.27),
                                                                    (-2.35, -1.43, 2.50, 3.27))]
    g.append(pane_y(-1.36, 0.17, 0.56, 0.56, 2.30))
    glass = unbaked_mesh(g, 'SM_NS_Barber_Front_Glass', 'ns_glass', ANCHOR, LOG)
    neon = glow(glow_parts, 'SM_NS_Barber_Front_GlowNeon', ANCHOR, LOG)
    return [obj, glass, neon]


# ------------------------------------------------------------------ the room

WALL_COL = 0xe6dcc3       # cream
MINT = 0x9fd3c7
TRIM = 0xece4cf           # moldings, a shade lighter
BACK_DOOR = (-1.80, -0.88)   # the back door's opening on the back wall (local y), 2.13 tall
ROOM_PATH = [(0.68, -2.40), (9.0, -2.40), (9.0, 2.45), (0.17, 2.45)]   # counter-clockwise: the room on the left


def draw_private(s, w, h):
    s.rect(0, 0, w, h, 0x1c1c1c, rough=0.35)
    s.rect(3, 3, w - 6, h - 6, 0xd8c08a, metal=0.8, rough=0.3)
    s.rect(5, 5, w - 10, h - 10, 0x1c1c1c, rough=0.35)
    s.mid('PRIVATE', w / 2, h * 0.5, h * 0.42, w - 24, 0xd8c08a, 'Black', tracking=1.3, metal=0.8, rough=0.3)


def draw_exit(s, w, h):
    s.rect(0, 0, w, h, 0xf4f2ec, rough=0.3)
    s.mid('EXIT', w / 2, h * 0.5, h * 0.62, w * 0.8, 0xd8261e, 'Black', tracking=1.1)
    s.poly([(w * 0.86, h * 0.5), (w * 0.8, h * 0.62), (w * 0.8, h * 0.38)], 0xd8261e)


def draw_ext_tag(s, w, h):
    s.rect(0, 0, w, h, 0xf2e6b8, rough=0.7)
    s.mid('INSPECTED', w / 2, h * 0.85, h * 0.1, w - 6, 0x1a1a1a, 'Black')
    for i in range(6):
        s.rect(4, h * 0.68 - i * h * 0.1, w - 8, 0.6, 0x555555, rough=0.7)
    s.circle(w / 2, h * 0.08, 3, 0x8a8a8a)


def draw_ext_label(s, w, h):
    s.rect(0, 0, w, h, 0xece8dc, rough=0.5)
    s.mid('FIRE EXTINGUISHER', w / 2, h * 0.84, h * 0.09, w - 6, 0xb3201a, 'Black')
    s.mid('ABC', w / 2, h * 0.6, h * 0.22, w - 6, 0x1a1a1a, 'Black')
    for i in range(4):
        s.rect(6, h * 0.36 - i * h * 0.08, w - 12, 0.8, 0x444444)


def shell():
    k = kit('nsbs', 'SM_NS_Barber_Shell', ('floor', 'tile', 'walls', 'ceiling', 'print'), dust=0.0, low=0.0, cavity=0.02, print_px=512)
    TEXTURE_SIZES.update({'nsbs_floor': 4096, 'nsbs_tile': 2048, 'nsbs_walls': 2048, 'nsbs_ceiling': 1024, 'nsbs_print': 512})
    k.tile_origin = (0.665, -2.40)
    k.tile_z0 = 0.10
    k.tin_origin = (0.317, -2.413)
    k.tile_slot('tile', 'wall', (1.2, 1.0), origin=(0.0, 0.0), z_range=(0.08, 1.20))
    k.tile_slot('ceiling', 'xy', (1.2192, 1.2192), origin=k.tin_origin, z=CEIL, down=True)
    k.paths = [(0.75, -1.85, 2.6, -0.1, 0.9), (2.6, -0.1, 4.6, -0.1, 0.8), (2.0, 0.4, 2.0, 1.5, 0.7), (4.1, 0.4, 4.1, 1.5, 0.7),
               (4.6, -0.1, 8.0, 0.4, 0.7), (8.5, -1.3, 7.5, 0.4, 0.6), (1.4, -1.6, 3.9, -1.6, 0.6)]
    k.panel('private', 0.20, 0.07, draw_private, density=1.5)
    k.panel('exit', 0.30, 0.15, draw_exit, glow=2.0)
    k.panel('ext_tag', 0.05, 0.09, draw_ext_tag, density=2.0)
    k.panel('ext_label', 0.07, 0.10, draw_ext_label, density=2.0)
    k.atlas.render()
    # The floor: 30 cm checker in 1.5 m pieces (seams in the grout).
    k.add(grid_plane('floor', 0.665, 9.0, -2.40, 2.45, 0.0, 1.5), 'checker', slot='floor')
    k.add(grid_plane('floor_b', 0.45, 0.665, -1.30, 2.45, 0.0, 1.5), 'checker', slot='floor')
    # Walls: the tiled wainscot, the painted walls above, cut round the back door.
    bd0, bd1 = BACK_DOOR
    t0, t1 = bd0 + 2.40, bd1 + 2.40
    for (p0, p1, nrm, holes_lo, holes_hi) in (
            ((0.17, 2.45), (9.0, 2.45), (0, -1), [], []),
            ((0.17, -2.40), (9.0, -2.40), (0, 1), [(0.0, 0.51, 0.0, 2.40)], [(0.0, 0.51, 0.0, 2.40)]),
            ((9.0, -2.40), (9.0, 2.45), (-1, 0), [(t0, t1, 0.0, 2.13)], [(t0, t1, 0.0, 2.13)])):
        k.add(wall_plane('wainscot', p0, p1, 0.08, 1.20, nrm, 2.0, holes_lo), 'subway', MINT, slot='tile')
        k.add(wall_plane('wall', p0, p1, 1.20, CEIL, nrm, 2.4, holes_hi), 'paint', WALL_COL, slot='walls')
        k.add(wall_plane('wall_low', p0, p1, 0.0, 0.08, nrm, 4.0, holes_lo), 'paint', 0x2a2a2a, slot='walls')
    # The cove base, the bullnose cap, a picture rail and the crown, round the three walls (broken at the back door).
    segs = [[(0.68, -2.40), (9.0, -2.40), (9.0, bd0 - 0.06)], [(9.0, bd1 + 0.06), (9.0, 2.45), (0.17, 2.45)]]
    cove = [(0.0, 0.0), (0.032, 0.0), (0.03, 0.004), (0.016, 0.012), (0.011, 0.03), (0.011, 0.095), (0.008, 0.1), (0.0, 0.1)]
    cap = [(0.0, 1.17), (0.010, 1.17), (0.019, 1.18), (0.022, 1.196), (0.019, 1.214), (0.011, 1.226), (0.0, 1.228)]
    for sg in segs:
        k.add(molding('cove', sg, cove), 'ceramic', 0x141414, slot='walls')
        k.add(molding('cap', sg, cap), 'ceramic', MINT, slot='walls')
    rail = [(0.0, 2.28), (0.016, 2.28), (0.022, 2.29), (0.024, 2.31), (0.018, 2.322), (0.012, 2.33), (0.0, 2.335)]
    crown = [(0.0, 3.13), (0.008, 3.13), (0.008, 3.145), (0.016, 3.152), (0.03, 3.17), (0.055, 3.205), (0.08, 3.235), (0.10, 3.255),
             (0.112, 3.27), (0.118, 3.282), (0.13, 3.284), (0.13, 3.30), (0.0, 3.30)]
    full = [(0.68, -2.40), (9.0, -2.40), (9.0, 2.45), (0.17, 2.45)]
    k.add(molding('rail', [(0.68, -2.40), (9.0, -2.40), (9.0, bd0 - 0.12)], rail), 'wood', 0x3a2414, slot='walls', aux=0)
    k.add(molding('rail', [(9.0, bd1 + 0.12), (9.0, 2.45), (0.17, 2.45)], rail), 'wood', 0x3a2414, slot='walls', aux=0)
    k.add(molding('crown', [(0.17, -2.40)] + full[1:], crown), 'enamel', TRIM, slot='walls')
    # The tin ceiling.
    k.add(grid_plane('ceiling', 0.17, 9.0, -2.40, 2.45, CEIL, 2.4, up=False), 'tin', 0xe8e0c8, slot='ceiling')
    # Ceiling fixtures: two supply diffusers, a smoke detector.
    for (cx, cy) in ((6.4, -0.9), (2.1, -1.5)):
        k.add(bx('diffuser', cx - 0.3, cx + 0.3, cy - 0.3, cy + 0.3, CEIL - 0.012, CEIL, 0.004), 'enamel', 0xece9e0, slot='walls')
        for j in range(4):
            r_ = 0.26 - j * 0.06
            k.add(bx('diff_ring', cx - r_, cx + r_, cy - r_, cy + r_, CEIL - 0.035 + j * 0.005, CEIL - 0.012, 0.003), 'enamel', 0xe2ded4, slot='walls')
        k.add(bx('diff_core', cx - 0.05, cx + 0.05, cy - 0.05, cy + 0.05, CEIL - 0.05, CEIL - 0.03, 0.003), 'enamel', 0xe2ded4, slot='walls')
    k.add(lathe_solid('smoke', [(0.0, CEIL - 0.045), (0.06, CEIL - 0.045), (0.065, CEIL - 0.03), (0.065, CEIL), (0.0, CEIL)], Matrix.Translation((5.6, 0.1, 0.0)), 40),
          'plastic', 0xefede6, slot='walls')
    # The back door: casing, slab, hinges, lever, closer, kick plate, the PRIVATE plate; the EXIT sign over it.
    dc = 0x2a3a30
    k.add(bx('bd_cas_l', 8.95, 9.0, bd0 - 0.08, bd0, 0.0, 2.21, 0.004), 'enamel', dc, slot='walls')
    k.add(bx('bd_cas_r', 8.95, 9.0, bd1, bd1 + 0.08, 0.0, 2.21, 0.004), 'enamel', dc, slot='walls')
    k.add(bx('bd_cas_t', 8.95, 9.0, bd0 - 0.08, bd1 + 0.08, 2.13, 2.21, 0.004), 'enamel', dc, slot='walls')
    for (y0, y1) in ((bd0, bd0 + 0.02), (bd1 - 0.02, bd1)):
        k.add(bx('bd_jamb', 8.97, 9.06, y0, y1, 0.0, 2.13), 'enamel', dc, slot='walls')
    k.add(bx('bd_head', 8.97, 9.06, bd0, bd1, 2.11, 2.13), 'enamel', dc, slot='walls')
    k.add(bx('bd_slab', 9.0, 9.045, bd0 + 0.02, bd1 - 0.02, 0.008, 2.11, 0.003), 'enamel', 0x2f4434, slot='walls')
    k.add(bx('bd_kick', 8.998, 9.0, bd0 + 0.04, bd1 - 0.04, 0.02, 0.27, 0.001), 'brass', slot='walls')
    k.add(bx('bd_push', 8.998, 9.0, bd1 - 0.15, bd1 - 0.05, 1.05, 1.45, 0.001), 'brass', slot='walls')
    for z in (0.25, 1.05, 1.85):
        k.add(parts.rod('bd_hinge', (8.998, bd0 + 0.022, z - 0.05), (8.998, bd0 + 0.022, z + 0.05), 0.007, 10), 'brass', slot='walls')
    lx, ly_ = 8.985, bd1 - 0.09
    k.add(parts.rod('lever_rose', (lx + 0.012, ly_, 1.0), (lx, ly_, 1.0), 0.026, 24), 'brass', slot='walls')
    k.add(tube('lever', bend([(lx, ly_, 1.0), (lx - 0.05, ly_, 1.0), (lx - 0.06, ly_ - 0.11, 1.0)], 0.02, 4), 0.008, 10), 'brass', slot='walls')
    k.add(bx('closer', 8.93, 8.995, bd0 + 0.08, bd0 + 0.38, 2.02, 2.09, 0.008), 'enamel', 0x6a6c6e, slot='walls')
    k.add(bx('closer_arm', 8.90, 8.93, bd0 + 0.30, bd0 + 0.50, 2.05, 2.065, 0.004), 'enamel', 0x6a6c6e, slot='walls')
    k.add(bx('private_plate', 8.996, 8.998, (bd0 + bd1) / 2 - 0.10, (bd0 + bd1) / 2 + 0.10, 1.535, 1.605), 'enamel', 0x1c1c1c, slot='walls')
    k.decal('private', (8.9955, (bd0 + bd1) / 2, 1.57), u=(0, -1, 0), v=(0, 0, 1), lift=0.0002)
    ex = (bd0 + bd1) / 2
    k.add(bx('exit_box', 8.93, 9.0, ex - 0.16, ex + 0.16, 2.30, 2.48, 0.006), 'plastic', 0xf1efe8, slot='walls')
    k.decal('exit', (8.9295, ex, 2.39), u=(0, -1, 0), v=(0, 0, 1), lift=0.0002)
    # The switch plate and thermostat by the door; outlets in the wainscot.
    k.add(bx('switch', 8.992, 9.0, bd1 + 0.14, bd1 + 0.21, 1.18, 1.30, 0.003), 'plastic', 0xeee9dc, slot='walls')
    k.add(bx('toggle', 8.986, 8.992, bd1 + 0.172, bd1 + 0.178, 1.235, 1.255, 0.001), 'plastic', 0xeee9dc, slot='walls')
    k.add(bx('thermostat', 8.975, 9.0, bd1 + 0.30, bd1 + 0.44, 1.42, 1.52, 0.012), 'plastic', 0xe9e5d8, slot='walls')
    for x in (1.2, 4.3, 7.1):
        k.add(bx('outlet', x - 0.035, x + 0.035, -2.40, -2.393, 0.30, 0.415, 0.003), 'plastic', 0xeee9dc, slot='walls')
        for dz in (0.335, 0.38):
            k.add(bx('outlet_face', x - 0.017, x + 0.017, -2.393, -2.388, dz - 0.013, dz + 0.013, 0.004), 'plastic', 0xe8e2d2, slot='walls')
    # The fire extinguisher on its bracket by the back door.
    fx, fy = 8.89, 0.55
    k.add(bx('ext_bracket', 8.985, 9.0, fy - 0.05, fy + 0.05, 1.02, 1.30, 0.003), 'iron', slot='walls')
    k.add(lathe_solid('ext_body', [(0.0, 0.92), (0.07, 0.92), (0.078, 0.94), (0.078, 1.30), (0.06, 1.37), (0.02, 1.39), (0.0, 1.39)],
                      Matrix.Translation((fx, fy, 0.0)), 40), 'enamel', 0xb5221b, slot='walls')
    k.add(lathe_solid('ext_valve', [(0.0, 1.39), (0.018, 1.39), (0.018, 1.46), (0.0, 1.46)], Matrix.Translation((fx, fy, 0.0)), 16), 'chrome', slot='walls')
    k.add(bx('ext_handle', fx - 0.08, fx + 0.03, fy - 0.012, fy + 0.012, 1.455, 1.47, 0.004), 'iron', slot='walls')
    k.add(tube('ext_hose', bend([(fx - 0.01, fy - 0.02, 1.42), (fx - 0.07, fy - 0.05, 1.36), (fx - 0.085, fy - 0.06, 1.12)], 0.05, 5), 0.009, 10), 'rubber', 0x151515, slot='walls')
    k.decal('ext_label', (fx - 0.078, fy, 1.13), u=(0, -1, 0), v=(0, 0, 1), lift=0.0006, nu=8, nv=1, disp=lambda s, t: math.sqrt(0.078 ** 2 - ((s - 0.5) * 0.07) ** 2) - 0.078)
    k.decal('ext_tag', (fx - 0.03, fy - 0.07, 1.35), u=(1, -1, 0), v=(0, 0, 1), lift=0.0)
    # hand grime round the back door's push plate and lever, the switch; a scuffed band behind the waiting chairs
    k.grime = [((8.99, bd1 - 0.10, 1.15), 0.45, 0.6), ((8.99, bd1 + 0.17, 1.24), 0.18, 0.7), ((0.75, -2.38, 1.1), 0.5, 0.5)]
    return [k.finish(40.0)]


# ------------------------------------------------------------------ the door, its bell, the grille, the pole

def doorsign_image():
    """ns_screen_doorsign's placeholder: the hanging card (outside OPEN, inside CLOSED) over the hours card."""
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0xf2efe6, rough=0.6)
        y0 = h * 0.45
        ch = h - y0
        s.rect(6, y0 + 6, w / 2 - 12, ch - 12, 0xf6f2e8, rough=0.6)
        s.rect(14, y0 + 14, w / 2 - 28, ch - 28, 0xc4231c, rough=0.5)
        s.mid('OPEN', w / 4, y0 + ch * 0.58, ch * 0.36, w / 2 - 50, 0xf6f2e8, 'Black', tracking=1.1)
        s.mid('COME IN', w / 4, y0 + ch * 0.24, ch * 0.12, w / 2 - 70, 0xf6f2e8, 'Bold', tracking=1.4)
        s.rect(w / 2 + 6, y0 + 6, w / 2 - 12, ch - 12, 0x1d1d1d, rough=0.5)
        s.mid('CLOSED', w * 0.75, y0 + ch * 0.58, ch * 0.3, w / 2 - 50, 0xf2efe6, 'Black', tracking=1.1)
        s.mid('SEE YOU SOON', w * 0.75, y0 + ch * 0.24, ch * 0.1, w / 2 - 70, 0xc9a24a, 'Bold', tracking=1.3)
        s.rect(0, 0, w, h * 0.4, 0xf4f1e8, rough=0.7)
        s.mid('FIFTH ST. BARBERS', w / 2, h * 0.34, h * 0.045, w - 60, 0x1d3a2c, 'Black', tracking=1.2)
        lines = [('TUE - THU', '10 AM - 10 PM'), ('FRI - SAT', '10 AM - MIDNIGHT'), ('SUN', '10 AM - 10 PM'), ('MON', 'CLOSED')]
        for i, (a, b) in enumerate(lines):
            y = h * 0.26 - i * h * 0.055
            s.text(a, w * 0.12, y, h * 0.032, 0x1a1a1a, 'Bold')
            s.text(b, w * 0.88, y, h * 0.032, 0xa3221c if b == 'CLOSED' else 0x1a1a1a, 'Bold', align='RIGHT')
    return render_image('ns_screen_doorsign', 400.0, 400.0, 1024, draw)


def door():
    """SM_NS_Barber_Door (PROP): the leaf, hinge axis at the origin, running along +y, its outside toward -X."""
    k = kit('nsbd', 'SM_NS_Barber_Door', ('body', 'ns_glass', 'ns_screen_doorsign'), anchor=(0.0, 0.0, 0.0), dust=0.1, low=0.3)
    TEXTURE_SIZES.update({'nsbd_body': 2048})
    screen_material('ns_screen_doorsign', doorsign_image(), lit=False, rough=0.6)
    W, T, Hh = 0.93, 0.045, 2.29
    y0, y1 = 0.005, 0.005 + W
    xo, xi = -T, 0.0
    E = lambda o: k.add(o, 'enamel', SASH)
    E(bx('stile_h', xo, xi, y0, y0 + 0.11, 0.01, Hh, 0.004))
    E(bx('stile_l', xo, xi, y1 - 0.11, y1, 0.01, Hh, 0.004))
    E(bx('rail_top', xo, xi, y0 + 0.11, y1 - 0.11, Hh - 0.12, Hh, 0.004))
    E(bx('rail_lock', xo, xi, y0 + 0.11, y1 - 0.11, 0.80, 0.92, 0.004))
    E(bx('rail_bot', xo, xi, y0 + 0.11, y1 - 0.11, 0.01, 0.30, 0.004))
    E(bx('panel', xo + 0.012, xi - 0.012, y0 + 0.10, y1 - 0.10, 0.29, 0.81))
    E(bx('field_out', xo + 0.006, xo + 0.013, y0 + 0.17, y1 - 0.17, 0.36, 0.74, 0.004))
    E(bx('field_in', xi - 0.013, xi - 0.006, y0 + 0.17, y1 - 0.17, 0.36, 0.74, 0.004))
    # glazing stops round the glass, both faces
    gz0, gz1 = 0.92, Hh - 0.12
    for xs in ((xo - 0.004, xo + 0.012), (xi - 0.012, xi + 0.004)):
        E(bx('stop_b', xs[0], xs[1], y0 + 0.11, y1 - 0.11, gz0, gz0 + 0.018, 0.003))
        E(bx('stop_t', xs[0], xs[1], y0 + 0.11, y1 - 0.11, gz1 - 0.018, gz1, 0.003))
        E(bx('stop_h', xs[0], xs[1], y0 + 0.11, y0 + 0.128, gz0, gz1, 0.003))
        E(bx('stop_l', xs[0], xs[1], y1 - 0.128, y1 - 0.11, gz0, gz1, 0.003))
    g = pane_x(-T / 2, y0 + 0.11, y1 - 0.11, gz0, gz1)
    k.special(g, 'ns_glass')
    # hardware: the brass kick plate (out), push plate and bar (in), the pull (out), the deadbolt, the hinges' leaves
    k.add(bx('kick', xo - 0.0015, xo, y0 + 0.02, y1 - 0.02, 0.025, 0.27, 0.001), 'brass')
    k.add(bx('kick_in', xi, xi + 0.0015, y0 + 0.02, y1 - 0.02, 0.025, 0.22, 0.001), 'brass')
    py = y1 - 0.055
    for z in (0.98, 1.30):
        k.add(parts.rod('pull_post', (xo, py, z), (xo - 0.055, py, z), 0.008, 12), 'brass')
    k.add(parts.rod('pull', (xo - 0.055, py, 0.96), (xo - 0.055, py, 1.32), 0.013, 16), 'brass')
    for yy in (y0 + 0.06, y1 - 0.06):
        k.add(parts.rod('push_post', (xi, yy, 1.06), (xi + 0.06, yy, 1.06), 0.008, 12), 'brass')
    k.add(parts.rod('push_bar', (xi + 0.06, y0 + 0.05, 1.06), (xi + 0.06, y1 - 0.05, 1.06), 0.014, 16), 'brass')
    k.add(parts.rod('bolt_out', (xo, y1 - 0.055, 1.42), (xo - 0.012, y1 - 0.055, 1.42), 0.026, 24), 'brass')
    k.add(parts.rod('bolt_key', (xo - 0.012, y1 - 0.055, 1.42), (xo - 0.016, y1 - 0.055, 1.42), 0.012, 16), 'brass')
    k.add(parts.rod('bolt_in', (xi, y1 - 0.055, 1.42), (xi + 0.01, y1 - 0.055, 1.42), 0.026, 24), 'brass')
    k.add(bx('bolt_turn', xi + 0.01, xi + 0.025, y1 - 0.065, y1 - 0.045, 1.395, 1.445, 0.004), 'brass')
    for z in (0.25, 1.15, 2.05):
        k.add(bx('hinge_leaf', xi - 0.001, xi + 0.0015, y0, y0 + 0.045, z - 0.05, z + 0.05, 0.001), 'brass')
    k.add(bx('sweep', xo - 0.006, xo, y0 + 0.01, y1 - 0.01, 0.002, 0.03, 0.002), 'rubber', 0x111111)
    # the hanging OPEN/CLOSED card on its chain (inside, in front of the glass): outside face u 0..0.5, inside 0.5..1
    cy, cz, cw, chh = (y0 + y1) / 2, 1.70, 0.30, 0.15
    cx0, cx1 = xi + 0.012, xi + 0.015
    out_f = quad('card_out', [(cx0, cy + cw / 2, cz - chh / 2), (cx0, cy - cw / 2, cz - chh / 2), (cx0, cy - cw / 2, cz + chh / 2), (cx0, cy + cw / 2, cz + chh / 2)])
    in_f = quad('card_in', [(cx1, cy - cw / 2, cz - chh / 2), (cx1, cy + cw / 2, cz - chh / 2), (cx1, cy + cw / 2, cz + chh / 2), (cx1, cy - cw / 2, cz + chh / 2)])
    for o, (ua, ub) in ((out_f, (0.0, 0.5)), (in_f, (0.5, 1.0))):
        uv = o.data.uv_layers.new(name='UVMap')
        for li, (a, b) in enumerate(((0, 0), (1, 0), (1, 1), (0, 1))):
            uv.data[li].uv = (ua + (ub - ua) * a, 0.45 + 0.55 * b)
        o.data.update()
        core.orient_normals(o, (-1, 0, 0) if o is out_f else (1, 0, 0))
        k.special(o, 'ns_screen_doorsign')
    k.add(bx('card_edge', cx0, cx1, cy - cw / 2, cy + cw / 2, cz - chh / 2 - 0.0005, cz - chh / 2), 'paper', 0xe8e4da)
    k.add(bx('card_edge_t', cx0, cx1, cy - cw / 2, cy + cw / 2, cz + chh / 2, cz + chh / 2 + 0.0005), 'paper', 0xe8e4da)
    hook = (xi + 0.014, cy, gz1 - 0.06)
    k.add(lathe_solid('suction', [(0.0, 0.0), (0.016, 0.0), (0.012, 0.006), (0.004, 0.012), (0.0, 0.016)],
                      Matrix.Translation((xi - 0.0225 + 0.0005, cy, gz1 - 0.06)) @ Matrix.Rotation(math.radians(90), 4, 'Y'), 24), 'plastic', 0xd8dcd8)
    for s_ in (-1, 1):
        a = Vector(hook)
        b = Vector((cx0 + 0.0015, cy + s_ * (cw / 2 - 0.03), cz + chh / 2))
        nlinks = 10
        for i in range(nlinks):
            p0 = a.lerp(b, i / nlinks)
            p1 = a.lerp(b, (i + 0.8) / nlinks)
            k.add(parts.rod('chain', tuple(p0), tuple(p1), 0.0012, 5), 'chrome')
    # the hours card, taped inside the glass, read from the street (u toward -y, v 0..0.4)
    hy0, hy1, hz0, hz1 = cy - 0.14, cy + 0.14, 1.24, 1.42
    hx = -T / 2 + 0.0025
    hc = quad('hours', [(hx, hy1, hz0), (hx, hy0, hz0), (hx, hy0, hz1), (hx, hy1, hz1)])
    uv = hc.data.uv_layers.new(name='UVMap')
    for li, (a, b) in enumerate(((0, 0), (1, 0), (1, 1), (0, 1))):
        uv.data[li].uv = (a, 0.4 * b)
    core.orient_normals(hc, (-1, 0, 0))
    k.special(hc, 'ns_screen_doorsign')
    k.add(bx('hours_back', hx + 0.0001, hx + 0.0006, hy0, hy1, hz0, hz1), 'paper', 0xf0ece2)
    for (ty, tz) in ((hy0, hz1), (hy1, hz1), (hy0, hz0), (hy1, hz0)):
        k.add(bx('tape', hx + 0.0006, hx + 0.0009, ty - 0.02, ty + 0.02, tz - 0.012, tz + 0.012), 'paper', 0xd9d0a8)
    k.polish = [((xo - 0.055, py, 1.14), 0.12, 0.8), ((xi + 0.06, (y0 + y1) / 2, 1.06), 0.3, 0.6)]
    k.grime = [((xo, y1 - 0.1, 1.15), 0.18, 0.7), ((xi, y1 - 0.1, 1.15), 0.18, 0.5), ((xo, (y0 + y1) / 2, 0.1), 0.4, 0.5)]
    return [k.finish(40.0)]


def door_bell():
    """SM_NS_DoorBell (PROP): a brass shop bell hanging on a coil spring; origin at the spring's root."""
    k = kit('nsbb', 'SM_NS_DoorBell', ('body',), anchor=(0.0, 0.0, 0.0), dust=0.0, low=0.0)
    TEXTURE_SIZES.update({'nsbb_body': 512})
    k.add(parts.rod('root', (0.0, 0.0, 0.006), (0.0, 0.0, -0.008), 0.006, 12), 'brass')
    k.add(parts.spring('spring', (0.0, 0.0, -0.008), (0.0, 0.0, -0.075), coil_r=0.006, wire_r=0.0012, pitch=0.0028, lead=0.006), 'brass')
    k.add(parts.rod('hanger', (0.0, 0.0, -0.075), (0.0, 0.0, -0.088), 0.004, 10), 'brass')
    prof = [(0.0, -0.088), (0.012, -0.088), (0.016, -0.092), (0.022, -0.103), (0.03, -0.12), (0.036, -0.128), (0.037, -0.133), (0.031, -0.132),
            (0.026, -0.125), (0.019, -0.112), (0.013, -0.098), (0.0, -0.096)]
    k.add(lathe_solid('bell', prof, None, 48), 'brass')
    k.add(parts.rod('clapper_rod', (0.0, 0.0, -0.096), (0.0, 0.0, -0.122), 0.0012, 6), 'brass')
    k.add(lathe_solid('clapper', [(0.0, -0.121), (0.004, -0.122), (0.006, -0.126), (0.004, -0.130), (0.0, -0.131)], None, 16), 'brass')
    return [k.finish(30.0)]


def grille():
    """SM_NS_Barber_Grille (PROP): the roll-down link grille; origin at the top middle, the curtain down to z -3.07."""
    k = kit('nsbg', 'SM_NS_Barber_Grille', ('body',), anchor=(0.0, 0.0, 0.0), dust=0.0, low=0.0)
    TEXTURE_SIZES.update({'nsbg_body': 1024})
    hw = 2.49
    Hh = GRILLE_TOP
    z = -0.05
    rows = []
    while z > -Hh + 0.10:
        rows.append(z)
        z -= 0.10
    for z in rows:
        for a, b in ((-hw, -hw / 3), (-hw / 3, hw / 3), (hw / 3, hw)):
            k.add(parts.rod('rod', (0.0, a, z), (0.0, b, z), 0.0055, 6), 'galv')
    # flat links between each pair of rods, every 27 cm, staggered row to row (a brick pattern)
    for i in range(len(rows) - 1):
        y = -hw + 0.06 + (0.135 if i % 2 else 0.0)
        while y < hw - 0.04:
            k.add(bx('link', -0.009, 0.009, y - 0.008, y + 0.008, rows[i + 1] - 0.012, rows[i] + 0.012), 'galv')
            y += 0.27
    for s_ in (-1, 1):
        k.add(bx('edge', -0.012, 0.012, s_ * hw - 0.012, s_ * hw + 0.012, -Hh + 0.06, -0.02, 0.003), 'galv')
    k.add(bx('bottom', -0.03, 0.03, -hw, hw, -Hh, -Hh + 0.07, 0.006), 'galv')
    k.add(bx('bottom_lip', -0.035, 0.0, -hw, hw, -Hh + 0.06, -Hh + 0.075, 0.003), 'galv')
    for y in (-1.2, 1.2):
        k.add(tube('handle', bend([(-0.03, y - 0.06, -Hh + 0.035), (-0.075, y - 0.06, -Hh + 0.035), (-0.075, y + 0.06, -Hh + 0.035),
                                   (-0.03, y + 0.06, -Hh + 0.035)], 0.02, 4), 0.008, 10), 'galv')
    k.add(bx('hasp', -0.06, -0.03, -0.03, 0.03, -Hh + 0.005, -Hh + 0.06, 0.004), 'iron')
    k.grime = [((0.0, y, -Hh + 0.4), 0.8, 0.6) for y in (-1.2, 1.2)]
    return [k.finish(35.0)]


def pole_texture():
    """ns_barberpole's still: red, white, blue, white climbing twice round per turn (u round, v meters up)."""
    W, Hh = 512, 1024
    u = (np.arange(W) + 0.5) / W
    v = (np.arange(Hh) + 0.5) / Hh
    U, V = np.meshgrid(u, v)
    ph = np.mod(2.0 * U + V * (1.5 / 0.62) * 2.0, 1.0)
    img = np.ones((Hh, W, 4), np.float32)
    cols = [(0.62, 0.03, 0.03), (0.92, 0.9, 0.86), (0.04, 0.1, 0.42), (0.92, 0.9, 0.86)]
    for i, cc in enumerate(cols):
        msk = (ph >= i / 4) & (ph < (i + 1) / 4)
        for ch in range(3):
            img[:, :, ch][msk] = cc[ch]
    path = os.path.join(core.BUILD_DIR, 'textures', 'ns_barberpole.png')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    im = bpy.data.images.new('ns_barberpole_img', W, Hh, alpha=False)
    im.pixels.foreach_set(img.ravel())
    im.filepath_raw = path
    im.file_format = 'PNG'
    im.save()
    bpy.data.images.remove(im)
    return path


def pole():
    """SM_NS_Barber_Pole (PROP): origin on the wall face under the pole at the ground; the pole's axis 0.30 out (-X)."""
    def make_insert(n):
        m = core.Mat(n)
        img = m.image(pole_texture())
        m.link(img.outputs['Color'], m.bsdf.inputs['Base Color'])
        m.link(img.outputs['Color'], m.bsdf.inputs['Emission Color'])
        m.set('Emission Strength', 0.6)
        m.set('Roughness', 0.35)
        return m.m
    unbaked('ns_barberpole', make_insert)
    k = kit('nsbp', 'SM_NS_Barber_Pole', ('body', 'ns_barberpole', 'ns_glass'), anchor=(0.0, 0.0, 0.0), dust=0.15, low=0.0)
    TEXTURE_SIZES.update({'nsbp_body': 1024})
    ax = -0.30
    M = Matrix.Translation((ax, 0.0, 0.0))
    # the insert (runtime stripes): u round from the wall side, v meters up from its foot
    ins = core.lathe('insert', [(0.074, 1.88), (0.074, 2.50)], segments=48, v_range=(0.0, 0.62))
    core.orient_normals(ins)
    ins.data.transform(M)
    k.special(ins, 'ns_barberpole')
    sleeve = core.lathe('sleeve', [(0.086, 1.855), (0.086, 2.525)], segments=48)
    core.orient_normals(sleeve)
    sleeve.data.transform(M)
    k.special(sleeve, 'ns_glass')
    k.add(lathe_solid('cap_lo', [(0.0, 1.70), (0.012, 1.70), (0.02, 1.715), (0.03, 1.74), (0.05, 1.77), (0.085, 1.80), (0.1, 1.815), (0.1, 1.84),
                                 (0.094, 1.845), (0.094, 1.86), (0.0, 1.86)], M, 48), 'chrome')
    k.add(lathe_solid('cap_lo_ring', [(0.0, 1.685), (0.006, 1.685), (0.009, 1.70), (0.0, 1.705)], M, 16), 'chrome')
    k.add(lathe_solid('cap_hi', [(0.0, 2.52), (0.094, 2.52), (0.094, 2.535), (0.1, 2.54), (0.1, 2.58), (0.104, 2.585), (0.104, 2.6), (0.085, 2.615),
                                 (0.05, 2.625), (0.03, 2.63), (0.03, 2.665), (0.0, 2.665)], M, 48), 'chrome')
    k.add(lathe_solid('globe_collar', [(0.0, 2.655), (0.045, 2.655), (0.05, 2.67), (0.045, 2.685), (0.0, 2.685)], M, 32), 'chrome')
    # the brackets and wall plate
    k.add(bx('plate', -0.012, 0.0, -0.05, 0.05, 1.74, 2.66, 0.004), 'chrome')
    for zz in (1.80, 2.56):
        k.add(tube('arm', bend([(-0.012, 0.0, zz), (ax + 0.10, 0.0, zz), (ax + 0.10, 0.0, zz + (0.04 if zz < 2 else -0.04))], 0.02, 4), 0.009, 12), 'chrome')
        k.add(tube('scroll', [(-0.012 - 0.01, 0.0, zz - 0.02 if zz < 2 else zz + 0.02)] +
                   [(-0.06 - 0.03 * math.cos(a), 0.0, (zz - 0.05 if zz < 2 else zz + 0.05) + 0.03 * math.sin(a) * (1 if zz < 2 else -1))
                    for a in np.linspace(0.0, 1.5 * math.pi, 10)], 0.005, 8), 'chrome')
    for zz in (1.76, 2.64):
        k.add(parts.rod('screw', (0.0, 0.0, zz), (-0.016, 0.0, zz), 0.006, 12), 'chrome')
    k.add(tube('conduit', bend([(-0.012, 0.03, 2.62), (-0.012, 0.03, 2.75), (0.0, 0.03, 2.75)], 0.03, 4), 0.006, 8), 'enamel', 0x24282a)
    k.grime = [((ax, 0.0, 1.78), 0.12, 0.4)]
    obj = k.finish(35.0)
    g = lathe_solid('globe', [(0.0, 2.67), (0.04, 2.675), (0.075, 2.69), (0.1, 2.72), (0.112, 2.76), (0.11, 2.8), (0.098, 2.84), (0.075, 2.87),
                              (0.04, 2.885), (0.0, 2.89)], M, 40)
    gl = glow([g], 'SM_NS_Barber_Pole_GlowWarm', (0.0, 0.0, 0.0), LOG)
    return [obj, gl]


# ------------------------------------------------------------------ the chair

OXBLOOD = 0x4f1618
PORCELAIN = 0xece6d6
SEAT_H = 0.55
HEAD = (0.19, 0.0, 1.27)


def tufted(name, half, r, M, buttons, depth=0.014, spread=0.024, crown=0.012, face_axis=2, sub=3):
    """An upholstered pad (half extents in its own frame), its face (+face_axis) pulled in at button points into
    dimples with folds between them, then placed by M. Returns (pad, [button objects])."""
    hx, hy, hz = half
    obj = bx(name, -hx, hx, -hy, hy, -hz, hz, r * 0.6, 1)
    core.subdivide(obj, sub)
    for v in obj.data.vertices:
        p = v.co
        if p[face_axis] > 0.0:
            a, b = [i for i in range(3) if i != face_axis]
            ha, hb = half[a], half[b]
            k = max(0.0, 1.0 - (p[a] / ha) ** 4) * max(0.0, 1.0 - (p[b] / hb) ** 4)
            dz = crown * k
            for (ba, bb) in buttons:
                d2 = (p[a] - ba) ** 2 + (p[b] - bb) ** 2
                dz -= depth * math.exp(-d2 / (spread * spread)) * k
            # the folds between neighbouring buttons: shallow creases
            p[face_axis] += dz * (p[face_axis] / half[face_axis])
    obj.data.transform(M)
    obj.data.shade_smooth()
    bts = []
    for (ba, bb) in buttons:
        c = [0.0, 0.0, 0.0]
        a, b = [i for i in range(3) if i != face_axis]
        c[a], c[b] = ba, bb
        c[face_axis] = half[face_axis] + crown - depth - 0.002
        o = parts.rbox('button', tuple(c), (0.016, 0.016, 0.016), 0.0079, 2)
        o.data.transform(M)
        bts.append(o)
    return obj, bts


def diamond(nx, ny, sx, sy):
    """Button positions in a diamond pattern: nx x ny staggered points spaced sx, sy."""
    pts = []
    for j in range(ny):
        off = 0.5 * sx if j % 2 else 0.0
        n = nx - (1 if j % 2 else 0)
        for i in range(n):
            pts.append(((i - (nx - 1) / 2) * sx + off, (j - (ny - 1) / 2) * sy))
    return pts


def chair():
    """SM_NS_Barber_Chair (PROP): a hydraulic barber chair, the sitter facing -X; origin on the floor at the pedestal."""
    k = kit('nsbc', 'SM_NS_Barber_Chair', ('body',), anchor=(0.0, 0.0, 0.0), dust=0.12, low=0.12)
    TEXTURE_SIZES.update({'nsbc_body': 2048})
    P = lambda o: k.add(o, 'porcelain', PORCELAIN)
    C = lambda o: k.add(o, 'chrome')
    V = lambda o: k.add(o, 'vinyl', OXBLOOD)
    # the base, its trim ring, the column and its bellows
    P(lathe_solid('base', [(0.0, 0.0), (0.30, 0.0), (0.302, 0.008), (0.296, 0.02), (0.28, 0.03), (0.22, 0.045), (0.14, 0.07), (0.105, 0.095),
                           (0.095, 0.11), (0.0, 0.11)], None, 64))
    C(tube('trim', [(0.302 * math.cos(a), 0.302 * math.sin(a), 0.012) for a in np.linspace(0, 2 * math.pi, 64, endpoint=False)], 0.006, 8, closed=True))
    C(lathe_solid('column', [(0.0, 0.10), (0.062, 0.10), (0.062, 0.335), (0.0, 0.335)], None, 32))
    bell_prof = [(0.0, 0.105)] + [(0.078 + 0.006 * math.cos(i * math.pi), 0.105 + i * 0.012) for i in range(9)] + [(0.0, 0.105 + 8 * 0.012)]
    k.add(lathe_solid('bellows', bell_prof, None, 40), 'rubber', 0x1a1a1a)
    # the seat housing, the front apron, the seat
    P(bx('housing', -0.235, 0.235, -0.225, 0.225, 0.33, 0.44, 0.035, 3))
    P(bx('apron', -0.27, -0.215, -0.20, 0.20, 0.22, 0.44, 0.022, 3))
    seat, bts = tufted('seat', (0.25, 0.255, 0.05), 0.035, Matrix.Translation((0.0, 0.0, 0.493)), diamond(3, 3, 0.15, 0.14), depth=0.013, crown=0.012)
    V(seat)
    for b in bts:
        V(b)
    rr = core.rounded_rect(-0.245, -0.25, 0.245, 0.25, 0.03, 4)
    k.add(tube('piping', [(x, y, 0.538) for x, y in rr], 0.0055, 8, closed=True), 'vinyl', 0x3e1013)
    # the footrest: chrome arms down to a plate with a ribbed rubber mat and a lip
    for s in (-1, 1):
        C(tube('foot_arm', bend([(-0.24, s * 0.15, 0.30), (-0.33, s * 0.15, 0.22), (-0.44, s * 0.15, 0.19)], 0.04, 5), 0.013, 12))
    k.add(bx('foot_plate', -0.64, -0.36, -0.21, 0.21, 0.165, 0.19, 0.008, 2), 'stainless', aux=1)
    k.add(bx('foot_lip', -0.655, -0.625, -0.21, 0.21, 0.165, 0.235, 0.01, 2), 'chrome')
    for i in range(7):
        x = -0.60 + i * 0.035
        k.add(bx('foot_rib', x - 0.008, x + 0.008, -0.18, 0.18, 0.19, 0.198, 0.003), 'rubber', 0x141414)
    # the arms: porcelain side panels, chrome frames, vinyl pads
    for s in (-1, 1):
        prof = core.rounded_profile([(-0.25, 0.40), (0.19, 0.40), (0.21, 0.63), (0.14, 0.71), (-0.21, 0.71), (-0.26, 0.62), (-0.25, 0.40)],
                                    [0, 0.02, 0.05, 0.04, 0.04, 0.05, 0], 4)
        o = prism_y('arm_panel', prof[:-1], -0.018, 0.018)
        o.data.transform(Matrix.Translation((0.0, s * 0.285, 0.0)))
        P(o)
        C(tube('arm_frame', bend([(0.21, s * 0.30, 0.735), (-0.25, s * 0.30, 0.735), (-0.31, s * 0.30, 0.70), (-0.30, s * 0.30, 0.61),
                                  (-0.25, s * 0.30, 0.60)], 0.035, 6), 0.012, 12))
        pad = cushion('arm_pad', -0.27, 0.20, s * 0.30 - 0.045, s * 0.30 + 0.045, 0.745, 0.79, 0.04, crown=0.006, sub=2)
        V(pad)
    # the back: a reclined tufted pad on a porcelain shell framed in chrome
    tilt = Matrix.Rotation(math.radians(12.0), 4, 'Y')
    BM = Matrix.Translation((0.285, 0.0, 0.84)) @ tilt
    back_pad, bts = tufted('back', (0.05, 0.235, 0.25), 0.04, BM, diamond(3, 4, 0.14, 0.13), depth=0.015, crown=0.014, face_axis=0)
    # tufted() pulls the +x face; the sitter's side is -x: mirror the pad in its own frame
    back_pad.data.transform(BM @ Matrix.Scale(-1, 4, (1, 0, 0)) @ BM.inverted())
    core.orient_normals(back_pad)
    V(back_pad)
    for b in bts:
        b.data.transform(BM @ Matrix.Scale(-1, 4, (1, 0, 0)) @ BM.inverted())
        V(b)
    shell_ = bx('back_shell', 0.03, 0.09, -0.25, 0.25, -0.27, 0.27, 0.03, 3)
    shell_.data.transform(BM)
    P(shell_)
    fr = core.rounded_rect(-0.25, -0.27, 0.25, 0.27, 0.05, 5)
    C(tube('back_frame', [tuple(BM @ Vector((0.035, y, z))) for y, z in fr], 0.009, 10, closed=True))
    # the back's hinge struts down to the housing
    for s in (-1, 1):
        C(tube('strut', [tuple(BM @ Vector((0.07, s * 0.17, -0.22))), (0.24, s * 0.17, 0.40)], 0.014, 12))
    # the headrest on its bar
    top = BM @ Vector((0.06, 0.0, 0.24))
    pad_c = Vector((0.335, 0.0, 1.25))
    C(tube('head_bar', [tuple(top), tuple(pad_c + Vector((0.03, 0.0, -0.03)))], 0.011, 12))
    C(bx('head_clamp', top.x - 0.025, top.x + 0.025, -0.03, 0.03, top.z - 0.02, top.z + 0.03, 0.008, 2))
    hp = cushion('head_pad', -0.045, 0.045, -0.15, 0.15, -0.055, 0.055, 0.045, crown=0.008, sub=2,
                 matrix=Matrix.Translation(pad_c) @ Matrix.Rotation(math.radians(8.0), 4, 'Y'))
    V(hp)
    paper = bx('head_paper', -0.0495, -0.0465, -0.13, 0.13, -0.045, 0.045)
    paper.data.transform(Matrix.Translation(pad_c) @ Matrix.Rotation(math.radians(8.0), 4, 'Y'))
    k.add(paper, 'paper', 0xf4f2ec)
    # the levers: recline (right, at the back) and the hydraulic pump pedal
    C(tube('recline', bend([(0.16, -0.235, 0.42), (0.20, -0.33, 0.50), (0.17, -0.37, 0.64)], 0.04, 4), 0.008, 10))
    k.add(lathe_solid('recline_knob', [(0.0, -0.022), (0.016, -0.018), (0.022, 0.0), (0.016, 0.018), (0.0, 0.022)],
                      Matrix.Translation((0.17, -0.37, 0.66)), 24), 'plastic', 0x111111)
    C(tube('pedal_arm', bend([(0.0, -0.055, 0.17), (0.04, -0.22, 0.13), (0.06, -0.34, 0.10)], 0.05, 4), 0.012, 12))
    k.add(bx('pedal', 0.0, 0.12, -0.40, -0.30, 0.085, 0.11, 0.008, 2), 'rubber', 0x151515)
    C(tube('release', bend([(-0.03, 0.055, 0.26), (-0.06, 0.24, 0.24), (-0.10, 0.30, 0.30)], 0.04, 4), 0.008, 10))
    k.add(lathe_solid('release_knob', [(0.0, -0.018), (0.014, -0.014), (0.018, 0.0), (0.014, 0.014), (0.0, 0.018)],
                      Matrix.Translation((-0.10, 0.30, 0.31)), 20), 'plastic', 0x111111)
    k.polish = [((0.0, 0.0, 0.55), 0.22, 0.9), ((-0.05, 0.30, 0.79), 0.12, 0.8), ((-0.05, -0.30, 0.79), 0.12, 0.8), ((0.22, 0.0, 0.9), 0.15, 0.6),
                ((-0.50, 0.0, 0.2), 0.15, 0.7)]
    k.grime = [((-0.29, 0.30, 0.65), 0.08, 0.5), ((-0.29, -0.30, 0.65), 0.08, 0.5), ((-0.5, 0.0, 0.19), 0.2, 0.5)]
    return [k.finish(35.0)]


# ------------------------------------------------------------------ the station (the backbar on the north wall)

WALNUT = 0x4a2a16
MARBLE = 0xe4e1da
ST_X = (1.55, 5.65)
ST_FRONT = 1.93
TOP_Z = 0.885
MIRRORS = [(1.65, 3.55), (3.65, 5.55)]
MIR_Z = (1.12, 2.32)
MIR_Y = 2.425


def p_label(lines, ground, ink, title=None, title_ground=None, title_ink=None, face='Black'):
    def draw(s, w, h):
        s.rect(0, 0, w, h, ground, rough=0.5)
        top = h
        if title:
            bh = h * 0.32
            s.rect(0, h - bh, w, bh, title_ground if title_ground is not None else ink, rough=0.45)
            s.mid(title, w / 2, h - bh / 2, bh * 0.62, w * 0.88, title_ink if title_ink is not None else ground, face)
            top = h - bh
        n = max(1, len(lines))
        for i, ln in enumerate(lines):
            s.mid(ln, w / 2, top - top * (i + 0.6) / (n + 0.2), min(top / (n + 0.6) * 0.6, h * 0.2), w * 0.86, ink, 'Bold')
    return draw


def draw_hot(s, w, h):
    s.rect(0, 0, w, h, 0x1a1a1a, rough=0.4)
    s.mid('HOT TOWELS', w / 2, h * 0.64, h * 0.26, w * 0.8, 0xe8e2d4, 'Black', tracking=1.2)
    s.mid('180°F', w / 2, h * 0.26, h * 0.2, w * 0.5, 0xd8892a, 'Bold')


def draw_lamp(color):
    def draw(s, w, h):
        s.circle(w / 2, h / 2, w / 2, color, rough=0.2)
    return draw


def draw_steri(s, w, h):
    s.rect(0, 0, w, h, 0xeeeeea, rough=0.4)
    s.mid('UV STERILIZER', w / 2, h * 0.55, h * 0.4, w * 0.9, 0x2a4a8a, 'Black', tracking=1.1)


def draw_uv(s, w, h):
    s.rect(0, 0, w, h, 0x8a6ae8, rough=0.2)
    for i in range(6):
        s.rect(0, h * (i + 0.4) / 6, w, h * 0.04, 0xd8ccff, rough=0.2)


def draw_dial(s, w, h):
    s.rect(0, 0, w, h, 0xe8c98a, rough=0.2)
    for i, f in enumerate(('54', '60', '70', '80', '100', '120', '140', '160')):
        x = w * (0.08 + 0.84 * i / 7)
        s.rect(x - 0.3, h * 0.45, 0.6, h * 0.2, 0x3a2a14)
        s.mid(f, x, h * 0.25, h * 0.2, w * 0.1, 0x3a2a14, 'Bold')
    s.rect(w * 0.62, h * 0.1, 0.8, h * 0.8, 0xc8261e)


def draw_book(s, w, h):
    s.rect(0, 0, w, h, 0x141414, rough=0.55)
    s.rect(w * 0.12, h * 0.55, w * 0.76, h * 0.3, 0xe8e2d4, rough=0.6)
    s.mid('STYLES', w / 2, h * 0.73, h * 0.11, w * 0.6, 0x1a1a1a, 'Black', tracking=1.3)
    s.mid('FIFTH ST. BARBERS', w / 2, h * 0.61, h * 0.045, w * 0.66, 0x7a1c1c, 'Bold', tracking=1.2)


def draw_strip(s, w, h):
    s.rect(0, 0, w, h, 0xf4f1ea, rough=0.5)
    s.mid('NECK STRIPS', w / 2, h * 0.6, h * 0.22, w * 0.86, 0x1f3b6a, 'Black')
    s.mid('5 ROLLS · 360 STRIPS', w / 2, h * 0.28, h * 0.12, w * 0.8, 0x444444, 'Bold')


def draw_clipper_logo(s, w, h):
    s.rect(0, 0, w, h, 0x1a1a1a, rough=0.4)
    s.mid('TRIMCO', w / 2, h * 0.55, h * 0.5, w * 0.9, 0xc9a24a, 'Black', metal=0.8, rough=0.3)


def wrap_cyl(k, key, cx, cy, z0, z1, r, a0=-0.8, a1=0.8, n=12, lift=0.0006):
    """A label wrapped round a vertical cylinder at (cx, cy), facing -y at angle 0 (a in radians round from -y)."""
    bm = bmesh.new()
    rows = []
    for zz in (z0, z1):
        rows.append([bm.verts.new((cx + (r + lift) * math.sin(a), cy - (r + lift) * math.cos(a), zz)) for a in np.linspace(a0, a1, n + 1)])
    for i in range(n):
        bm.faces.new((rows[0][i], rows[0][i + 1], rows[1][i + 1], rows[1][i]))
    o = core.mesh_object('label', bm)
    k.wrap_print(o, key, lambda co: ((math.atan2(co.x - cx, -(co.y - cy)) - a0) / (a1 - a0), (co.z - z0) / (z1 - z0)))
    return o


def comb(k, M, length=0.18, col=0x121212, teeth=24):
    """A barber's comb in its own frame (x along it, teeth toward -y), placed by M."""
    parts_ = [bx('comb_spine', 0.0, length, -0.004, 0.006, 0.0, 0.0028, 0.0012)]
    for i in range(teeth):
        x = 0.006 + (length - 0.012) * i / (teeth - 1)
        parts_.append(bx('tooth', x - 0.0011, x + 0.0011, -0.026 if i < teeth // 2 else -0.022, -0.003, 0.0004, 0.0024))
    for o in parts_:
        o.data.transform(M)
        k.add(o, 'plastic', col)


def clipper(k, M, col=0x1a1a1a, logo=None):
    """Clippers in their own frame: the body along +z (the blades at z 0, the tail at z 0.165), front toward -y."""
    body = lathe_solid('clip_body', [(0.0, 0.012), (0.022, 0.014), (0.026, 0.04), (0.027, 0.10), (0.024, 0.15), (0.016, 0.165), (0.0, 0.168)], None, 24)
    body.data.transform(Matrix.Scale(0.75, 4, (0, 1, 0)))
    blade = bx('clip_blade', -0.024, 0.024, -0.012, 0.012, -0.004, 0.016, 0.003)
    teeth = bx('clip_teeth', -0.023, 0.023, -0.003, 0.003, -0.012, -0.004)
    lever = bx('clip_lever', 0.022, 0.032, -0.006, 0.006, 0.05, 0.09, 0.004)
    tail = parts.rod('clip_tail', (0.0, 0.0, 0.165), (0.0, 0.0, 0.19), 0.006, 10)
    for o, f, c in ((body, 'plastic', col), (blade, 'chrome', 0xffffff), (teeth, 'chrome', 0xffffff), (lever, 'chrome', 0xffffff), (tail, 'rubber', 0x111111)):
        o.data.transform(M)
        k.add(o, f, c)
    if logo:
        k.decal(logo, tuple(M @ Vector((0.0, -0.0205, 0.11))), u=tuple((M.to_3x3() @ Vector((1, 0, 0)))), v=tuple((M.to_3x3() @ Vector((0, 0, 1)))),
                w=0.03, h=0.012, lift=0.0004)
    return M @ Vector((0.0, 0.0, 0.19))


def coil_cord(k, a, b, coil_r=0.008, pitch=0.009, wire=0.0022, sag=0.06):
    """A coiled clipper cord from a to b, sagging between."""
    A, B = Vector(a), Vector(b)
    L = (B - A).length
    turns = int(L / pitch)
    pts = []
    for i in range(turns * 8 + 1):
        t = i / (turns * 8)
        c = A.lerp(B, t) + Vector((0, 0, -sag * 4 * t * (1 - t)))
        d = (B - A).normalized()
        u = d.cross(Vector((0, 0, 1)))
        if u.length < 1e-3:
            u = Vector((1, 0, 0))
        u.normalize()
        w = d.cross(u)
        ang = 2 * math.pi * turns * t
        pts.append(tuple(c + u * coil_r * math.cos(ang) + w * coil_r * math.sin(ang)))
    k.add(tube('cord', pts, wire, 6), 'rubber', 0x151515)


def station():
    k = kit('nsbt', 'SM_NS_Barber_Station', ('body', 'print', 'ns_glass'), dust=0.12, low=0.12, print_px=1024)
    TEXTURE_SIZES.update({'nsbt_body': 4096, 'nsbt_print': 1024})
    k.panel('talc', 0.12, 0.08, p_label(['FOR AFTER THE CUT', '8 OZ'], 0xf2ede0, 0x7a1c1c, 'BARBER TALC', 0x7a1c1c, 0xf2ede0))
    k.panel('tonic0', 0.10, 0.07, p_label(['HAIR TONIC', 'BAY RUM'], 0xe9dfc6, 0x3a2412, 'PRUITT\'S', 0x7a1c1c, 0xf2e6c8))
    k.panel('tonic1', 0.10, 0.07, p_label(['AFTERSHAVE', 'LIME'], 0xe0e8d0, 0x1f3a1c, 'NO. 9', 0x1f3a1c, 0xe0e8d0))
    k.panel('tonic2', 0.10, 0.07, p_label(['SCALP', 'TREATMENT'], 0xf0ece0, 0x1f3b6a, 'KLEAR', 0x1f3b6a, 0xf0ece0))
    k.panel('hot', 0.14, 0.05, draw_hot, density=1.5)
    k.panel('hot_lamp', 0.012, 0.012, draw_lamp(0xff8a1c), glow=4.0, density=3.0)
    k.panel('steri', 0.12, 0.025, draw_steri, density=1.5)
    k.panel('uv', 0.22, 0.06, draw_uv, glow=1.5)
    k.panel('dial', 0.16, 0.035, draw_dial, glow=0.8, density=1.5)
    k.panel('book', 0.25, 0.30, draw_book)
    k.panel('strip', 0.10, 0.05, draw_strip, density=1.5)
    k.panel('logo', 0.06, 0.024, draw_clipper_logo, density=2.0)
    k.atlas.render()
    x0, x1 = ST_X
    W_ = lambda o, aux=0: k.add(o, 'wood', WALNUT, aux=aux)
    # the carcass, toe kick, door and drawer fronts with brass pulls
    k.add(bx('toe', x0 + 0.03, x1 - 0.03, ST_FRONT + 0.07, 2.45, 0.0, 0.10), 'enamel', 0x161616)
    W_(bx('carcass', x0, x1, ST_FRONT, 2.45, 0.10, TOP_Z - 0.035, 0.004))
    for s0 in (x0, x0 + 2.05):
        fronts = [('door', s0 + 0.06, s0 + 0.64), ('drw', s0 + 0.68, s0 + 1.28), ('door', s0 + 1.32, s0 + 1.98)]
        for kind, a, b in fronts:
            if kind == 'door':
                W_(bx('door', a, b, ST_FRONT - 0.02, ST_FRONT, 0.13, 0.83, 0.004))
                W_(bx('door_panel', a + 0.07, b - 0.07, ST_FRONT - 0.03, ST_FRONT - 0.02, 0.21, 0.75, 0.006), 2)
                kx = b - 0.05 if a < s0 + 0.5 else a + 0.05
                k.add(lathe_solid('knob', [(0.0, 0.0), (0.008, 0.0), (0.006, 0.012), (0.014, 0.022), (0.012, 0.03), (0.0, 0.032)],
                                  Matrix.Translation((kx, ST_FRONT - 0.02, 0.62)) @ Matrix.Rotation(math.radians(90), 4, 'X'), 20), 'brass')
            else:
                for (za, zb) in ((0.13, 0.355), (0.37, 0.595), (0.61, 0.83)):
                    W_(bx('drawer', a, b, ST_FRONT - 0.02, ST_FRONT, za, zb, 0.005))
                    cx_, cz_ = (a + b) / 2, (za + zb) / 2 + 0.03
                    cup = lathe_shell('pull', [(0.035, 0.0), (0.036, 0.004), (0.03, 0.02), (0.02, 0.026)], None, 24)
                    cup.data.transform(Matrix.Translation((cx_, ST_FRONT - 0.024, cz_)) @ Matrix.Rotation(math.radians(90), 4, 'X') @
                                       Matrix.Scale(1.0, 4) @ Matrix.Rotation(math.radians(180), 4, 'Z'))
                    solidify(cup, 0.0025)
                    k.add(cup, 'brass')
                    for dx in (-0.045, 0.045):
                        k.add(parts.rod('pull_screw', (cx_ + dx, ST_FRONT - 0.02, cz_ + 0.012), (cx_ + dx, ST_FRONT - 0.026, cz_ + 0.012), 0.005, 10), 'brass')
    # the marble top, backsplash, shelf and brackets
    k.add(bx('top', x0 - 0.02, x1 + 0.02, ST_FRONT - 0.07, 2.45, TOP_Z - 0.035, TOP_Z, 0.008, 3), 'marble', MARBLE)
    k.add(bx('splash', x0, x1, 2.43, 2.45, TOP_Z, 1.08, 0.003), 'marble', MARBLE)
    k.add(bx('shelf', x0, x1, 2.30, 2.45, 1.08, 1.105, 0.006, 2), 'marble', MARBLE)
    for bxp in np.linspace(x0 + 0.2, x1 - 0.2, 6):
        k.add(tube('bracket', bend([(bxp, 2.43, 1.0), (bxp, 2.43, 1.075), (bxp, 2.33, 1.075)], 0.02, 3), 0.006, 8), 'chrome')
    # the mirror frame: backboard, reeded pilasters, the cornice, beads round the mirrors
    W_(bx('backboard', x0, x1, 2.435, 2.45, 1.105, 2.50), 2)
    for (a, b) in ((x0, x0 + 0.10), (3.55, 3.65), (5.55, x1)):
        W_(bx('pilaster', a, b, 2.37, 2.45, 1.105, 2.34, 0.004), 2)
        for i in range(3):
            xx = a + (b - a) * (i + 1) / 4
            W_(parts.rod('reed', (xx, 2.37, 1.16), (xx, 2.37, 2.29), 0.009, 10), 2)
        W_(bx('capital', a - 0.01, b + 0.01, 2.35, 2.45, 2.30, 2.34, 0.004))
        W_(bx('plinth', a - 0.005, b + 0.005, 2.36, 2.45, 1.105, 1.16, 0.004))
    W_(core.prism_x('cornice', [(2.45, 2.34), (2.36, 2.34), (2.36, 2.36), (2.335, 2.385), (2.335, 2.41), (2.30, 2.45), (2.29, 2.47), (2.29, 2.50),
                                (2.45, 2.50)], x0 - 0.04, x1 + 0.04))
    for (a, b) in MIRRORS:
        W_(bx('bead_b', a, b, 2.41, 2.43, MIR_Z[0] - 0.015, MIR_Z[0] + 0.005, 0.003))
        W_(bx('bead_t', a, b, 2.41, 2.43, MIR_Z[1] - 0.005, MIR_Z[1] + 0.015, 0.003))
    # --- chair 1's station (x 1.55..3.60)
    # the jar of blue disinfectant, no label, combs standing in it, its lid set aside
    jx, jy = 1.92, 2.22
    k.add(lathe_solid('jar_base', [(0.0, TOP_Z), (0.064, TOP_Z), (0.066, TOP_Z + 0.004), (0.064, TOP_Z + 0.014), (0.0, TOP_Z + 0.014)],
                      Matrix.Translation((jx, jy, 0.0)), 40), 'glassdark', 0x6a8a90)
    k.add(lathe_solid('jar_liquid', [(0.0, TOP_Z + 0.014), (0.0585, TOP_Z + 0.014), (0.0585, TOP_Z + 0.17), (0.05, TOP_Z + 0.172), (0.0, TOP_Z + 0.173)],
                      Matrix.Translation((jx, jy, 0.0)), 40), 'liquid', 0x1450b8)
    jg = core.lathe('jar_glass', [(0.0615, TOP_Z + 0.014), (0.0615, TOP_Z + 0.255), (0.0605, TOP_Z + 0.262), (0.0575, TOP_Z + 0.262), (0.0585, TOP_Z + 0.255),
                                  (0.0585, TOP_Z + 0.02)], segments=40)
    jg.data.transform(Matrix.Translation((jx, jy, 0.0)))
    k.special(jg, 'ns_glass')
    k.add(lathe_solid('jar_lid', [(0.0, 0.0), (0.066, 0.0), (0.066, 0.02), (0.055, 0.03), (0.012, 0.034), (0.012, 0.05), (0.0, 0.05)],
                      Matrix.Translation((jx + 0.13, jy + 0.04, TOP_Z)), 40), 'chrome')
    for i, (dx, dy, tilt, col) in enumerate(((-0.02, 0.0, 6, 0x111111), (0.015, 0.012, -5, 0x1a1a1a), (0.0, -0.018, 3, 0x6a1a14))):
        comb(k, Matrix.Translation((jx + dx, jy + dy, TOP_Z + 0.105)) @ Matrix.Rotation(math.radians(tilt), 4, 'Y') @ Matrix.Rotation(math.radians(-90), 4, 'Y')
             @ Matrix.Rotation(math.radians(90 * i), 4, 'X'), 0.19, col)
    # a spray bottle, the talc, the neck duster, a towel with combs and the straight razor
    sx, sy = 2.13, 2.30
    k.add(lathe_solid('spray', [(0.0, TOP_Z), (0.035, TOP_Z), (0.037, TOP_Z + 0.01), (0.037, TOP_Z + 0.15), (0.03, TOP_Z + 0.175), (0.013, TOP_Z + 0.19),
                                (0.013, TOP_Z + 0.205), (0.0, TOP_Z + 0.205)], Matrix.Translation((sx, sy, 0.0)), 32), 'gloss', 0x6a9ab8)
    k.add(bx('trigger_head', sx - 0.016, sx + 0.016, sy - 0.07, sy + 0.012, TOP_Z + 0.205, TOP_Z + 0.24, 0.008), 'plastic', 0x222222)
    k.add(bx('trigger', sx - 0.008, sx + 0.008, sy - 0.06, sy - 0.045, TOP_Z + 0.17, TOP_Z + 0.21, 0.004), 'plastic', 0x222222)
    tx, ty = 2.30, 2.34
    k.add(lathe_solid('talc', [(0.0, TOP_Z), (0.032, TOP_Z), (0.032, TOP_Z + 0.14), (0.026, TOP_Z + 0.155), (0.0, TOP_Z + 0.16)],
                      Matrix.Translation((tx, ty, 0.0)), 32), 'enamel', 0xf2ede0)
    wrap_cyl(k, 'talc', tx, ty, TOP_Z + 0.03, TOP_Z + 0.11, 0.032, -1.2, 1.2)
    dx_, dy_ = 2.48, 2.08
    dm = Matrix.Translation((dx_, dy_, TOP_Z + 0.02)) @ Matrix.Rotation(math.radians(15), 4, 'Z') @ Matrix.Rotation(math.radians(90), 4, 'Y')
    k.add(lathe_solid('duster_handle', [(0.0, 0.0), (0.014, 0.0), (0.018, 0.04), (0.016, 0.10), (0.012, 0.11), (0.0, 0.112)], dm, 20), 'wood', 0x2a1a10, aux=2)
    fan = lathe_solid('duster', [(0.0, -0.002), (0.02, 0.0), (0.045, -0.04), (0.05, -0.08), (0.03, -0.10), (0.0, -0.102)], dm, 24)
    fan.data.transform(dm @ Matrix.Scale(0.45, 4, (0, 1, 0)) @ dm.inverted())
    k.add(fan, 'felt', 0xe8dcc6)
    k.add(bx('towel', 2.68, 3.12, 1.98, 2.24, TOP_Z, TOP_Z + 0.012, 0.004), 'cloth', 0xf2f0ea)
    for i in range(3):
        comb(k, Matrix.Translation((2.72 + i * 0.012, 2.04 + i * 0.05, TOP_Z + 0.012 + 0.003 * i)) @ Matrix.Rotation(math.radians(4 - 3 * i), 4, 'Z'),
             0.18, (0x111111, 0x2a2a2a, 0x5a1612)[i])
    rm = Matrix.Translation((2.95, 2.16, TOP_Z + 0.013)) @ Matrix.Rotation(math.radians(-20), 4, 'Z')
    for o, f, c in ((bx('razor_blade', -0.07, 0.0, -0.011, 0.011, 0.0, 0.004, 0.001), 'chrome', 0xffffff),
                    (bx('razor_handle', 0.002, 0.12, -0.009, 0.009, 0.0, 0.012, 0.004), 'plastic', 0x0e0e0e),
                    (parts.rod('razor_pin', (0.002, 0.0, -0.001), (0.002, 0.0, 0.014), 0.003, 10), 'brass', 0xffffff)):
        o.data.transform(rm)
        k.add(o, f, c)
    # clippers hanging on hooks under the shelf, coiled cords down to a power strip
    for i, (hx_, col) in enumerate(((3.18, 0x1a1a1a), (3.34, 0x6a6a6e))):
        k.add(tube('hook', bend([(hx_, 2.31, 1.08), (hx_, 2.28, 1.08), (hx_, 2.28, 1.07)], 0.008, 3), 0.003, 8), 'chrome')
        clipper(k, Matrix.Translation((hx_, 2.28, 0.889)), col, 'logo' if i == 0 else None)
        coil_cord(k, (hx_, 2.28, 1.079), (3.47, 2.42, 0.93), sag=0.05)
    k.add(bx('strip', 3.42, 3.56, 2.405, 2.43, 0.90, 0.95, 0.006), 'plastic', 0xefece4)
    k.add(bx('strips_box', 3.26, 3.36, 2.24, 2.36, TOP_Z, TOP_Z + 0.06, 0.004), 'paper', 0xf4f1ea)
    k.decal('strip', (3.31, 2.2395, TOP_Z + 0.03), lift=0.0003, w=0.095, h=0.048)
    # the strop hanging on the cabinet's west end
    strop = bx('strop', x0 - 0.012, x0 - 0.006, 2.17, 2.23, 0.40, 0.84, 0.002)
    k.add(strop, 'vinyl', 0x5a3418)
    k.add(bx('strop_canvas', x0 - 0.016, x0 - 0.012, 2.175, 2.225, 0.40, 0.80, 0.002), 'cloth', 0xc8b896)
    k.add(tube('strop_hook', bend([(x0, 2.2, 0.83), (x0 - 0.03, 2.2, 0.83), (x0 - 0.03, 2.2, 0.86)], 0.01, 3), 0.004, 8), 'brass')
    # tonics on the shelf
    for i, (bx_, col) in enumerate(((1.80, 0x6a3a12), (1.92, 0x2a5a2a), (2.05, 0x2a3a5a))):
        k.add(lathe_solid('tonic', [(0.0, 1.105), (0.03, 1.105), (0.031, 1.115), (0.031, 1.24), (0.016, 1.28), (0.011, 1.30), (0.0, 1.30)],
                          Matrix.Translation((bx_, 2.375, 0.0)), 28), 'glassdark', col)
        k.add(lathe_solid('tonic_cap', [(0.0, 1.30), (0.013, 1.30), (0.013, 1.33), (0.0, 1.33)], Matrix.Translation((bx_, 2.375, 0.0)), 16), 'enamel',
              0x1a1a1a if i else 0xc9a24a)
        wrap_cyl(k, f'tonic{i}', bx_, 2.375, 1.14, 1.21, 0.031, -1.0, 1.0)
    # --- chair 2's station (x 3.60..5.65): the sterilizer, towels, the style book, a clipper, the hot-towel cabinet
    k.add(bx('steri', 3.76, 4.06, 2.14, 2.42, TOP_Z, TOP_Z + 0.17, 0.012, 2), 'enamel', 0xeeeeea)
    k.add(bx('steri_win', 3.79, 4.03, 2.135, 2.14, TOP_Z + 0.03, TOP_Z + 0.11, 0.003), 'glassdark', 0x2a2440)
    k.decal('uv', (3.91, 2.1345, TOP_Z + 0.07), w=0.22, h=0.06, lift=0.0003)
    k.decal('steri', (3.91, 2.1395, TOP_Z + 0.14), w=0.12, h=0.025, lift=0.0003)
    k.add(bx('steri_handle', 3.86, 3.96, 2.115, 2.135, TOP_Z + 0.115, TOP_Z + 0.125, 0.004), 'chrome')
    for i in range(4):
        z = TOP_Z + i * 0.026
        tw = cushion('towel', 4.26 + 0.01 * (i % 2), 4.56 - 0.006 * i, 2.12, 2.40, z, z + 0.026, 0.02, crown=0.003, sub=2)
        k.add(tw, 'cloth', 0xf2f0ea if i != 2 else 0xe9e4da)
    bm_ = Matrix.Translation((4.88, 2.13, TOP_Z)) @ Matrix.Rotation(math.radians(-7), 4, 'Z')
    for o, f, c in ((bx('book', -0.13, 0.13, -0.155, 0.155, 0.003, 0.042, 0.004), 'vinyl', 0x141414),
                    (bx('book_pages', -0.122, 0.128, -0.15, 0.15, 0.006, 0.039, 0.001), 'paper', 0xefe9da)):
        o.data.transform(bm_)
        k.add(o, f, c)
    k.decal('book', tuple(bm_ @ Vector((0.0, 0.0, 0.0425))), u=tuple(bm_.to_3x3() @ Vector((1, 0, 0))), v=tuple(bm_.to_3x3() @ Vector((0, 1, 0))),
            w=0.25, h=0.30, lift=0.0003)
    k.add(tube('hook2', bend([(5.02, 2.31, 1.08), (5.02, 2.28, 1.08), (5.02, 2.28, 1.07)], 0.008, 3), 0.003, 8), 'chrome')
    clipper(k, Matrix.Translation((5.02, 2.28, 0.889)), 0x2a2a2e)
    coil_cord(k, (5.02, 2.28, 1.079), (5.42, 2.42, 0.93), sag=0.04)
    k.add(bx('strip2', 5.36, 5.50, 2.405, 2.43, 0.90, 0.95, 0.006), 'plastic', 0xefece4)
    hx0, hx1 = 5.18, 5.56
    k.add(bx('hot_body', hx0, hx1, 2.10, 2.43, TOP_Z, TOP_Z + 0.36, 0.012, 2), 'stainless', aux=2)
    k.add(bx('hot_door', hx0 + 0.02, hx1 - 0.02, 2.09, 2.10, TOP_Z + 0.03, TOP_Z + 0.33, 0.006), 'stainless', aux=1)
    k.add(bx('hot_handle', hx0 + 0.13, hx1 - 0.13, 2.065, 2.08, TOP_Z + 0.29, TOP_Z + 0.305, 0.005), 'chrome')
    for xx in (hx0 + 0.15, hx1 - 0.15):
        k.add(parts.rod('hot_post', (xx, 2.09, TOP_Z + 0.297), (xx, 2.072, TOP_Z + 0.297), 0.004, 8), 'chrome')
    k.decal('hot', ((hx0 + hx1) / 2, 2.0895, TOP_Z + 0.20), w=0.14, h=0.05, lift=0.0003)
    k.decal('hot_lamp', (hx1 - 0.05, 2.0895, TOP_Z + 0.08), w=0.012, h=0.012, lift=0.0004)
    for xx in (hx0 + 0.03, hx1 - 0.03):
        k.add(bx('hot_foot', xx - 0.012, xx + 0.012, 2.12, 2.41, TOP_Z, TOP_Z + 0.006, 0.002), 'rubber', 0x111111)
    # the radio on the shelf
    rx, ry = 4.70, 2.37
    k.add(bx('radio', rx - 0.13, rx + 0.13, ry - 0.065, ry + 0.07, 1.105, 1.265, 0.025, 3), 'gloss', 0xe2d4b4)
    k.add(bx('radio_grille', rx - 0.115, rx - 0.005, ry - 0.067, ry - 0.06, 1.13, 1.24, 0.01), 'cloth', 0x6a4a2a)
    k.add(bx('radio_dial_bezel', rx + 0.005, rx + 0.115, ry - 0.068, ry - 0.06, 1.18, 1.235, 0.006), 'brass')
    k.decal('dial', (rx + 0.06, ry - 0.0685, 1.2075), w=0.1, h=0.03, lift=0.0002)
    for kx_ in (rx + 0.03, rx + 0.09):
        k.add(parts.rod('radio_knob', (kx_, ry - 0.06, 1.15), (kx_, ry - 0.08, 1.15), 0.013, 20), 'plastic', 0x4a2a14)
    k.add(tube('radio_cord', bend([(rx + 0.12, ry + 0.06, 1.12), (rx + 0.2, ry + 0.07, 1.11), (rx + 0.25, 2.44, 1.06), (rx + 0.25, 2.44, 0.95)], 0.03, 4),
               0.003, 6), 'rubber', 0x2a2a2a)
    k.grime = [((2.60, ST_FRONT, TOP_Z), 0.5, 0.5), ((4.60, ST_FRONT, TOP_Z), 0.5, 0.5), ((2.0, ST_FRONT, 0.55), 0.3, 0.4), ((4.0, ST_FRONT, 0.55), 0.3, 0.4)]
    k.polish = [((2.60, 2.0, TOP_Z), 0.6, 0.5), ((4.60, 2.0, TOP_Z), 0.6, 0.5)]
    return [k.finish(35.0)]


def mirror():
    """SM_NS_Barber_Mirror: two beveled panels over the station, ns_mirror, UV 0..1 across each as seen."""
    unbaked('ns_mirror')
    objs = []
    bev = 0.025
    for (a, b) in MIRRORS:
        z0, z1 = MIR_Z
        y = MIR_Y
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new('UVMap')
        outer = [(a, z0), (b, z0), (b, z1), (a, z1)]
        inner = [(a + bev, z0 + bev), (b - bev, z0 + bev), (b - bev, z1 - bev), (a + bev, z1 - bev)]
        vo = [bm.verts.new((x, y + 0.004, z)) for x, z in outer]
        vi = [bm.verts.new((x, y, z)) for x, z in inner]
        faces = [vi]
        for i in range(4):
            faces.append([vo[i], vo[(i + 1) % 4], vi[(i + 1) % 4], vi[i]])
        for fv in faces:
            f = bm.faces.new(fv)
            for l in f.loops:
                l[uvl].uv = ((l.vert.co.x - a) / (b - a), (l.vert.co.z - z0) / (z1 - z0))
        bm.normal_update()
        for f in bm.faces:
            if f.normal.y > 0:
                f.normal_flip()
        objs.append(core.mesh_object('mirror', bm))
    return [unbaked_mesh(objs, 'SM_NS_Barber_Mirror', 'ns_mirror', ANCHOR, LOG)]


# ------------------------------------------------------------------ the waiting chairs

def draw_mag(title, sub, ground, ink, accent, seed):
    def draw(s, w, h):
        r = random.Random(seed)
        s.rect(0, 0, w, h, ground, rough=0.3)
        s.rect(w * 0.06, h * 0.08, w * 0.88, h * 0.62, accent, rough=0.3)
        for _ in range(5):
            x, y = r.uniform(w * 0.1, w * 0.8), r.uniform(h * 0.12, h * 0.6)
            s.circle(x, y, r.uniform(w * 0.05, w * 0.16), r.choice([0x2a2a2a, 0xc8a070, 0x8a4a2a, 0xe8e0d0]), rough=0.3)
        s.mid(title, w / 2, h * 0.86, h * 0.13, w * 0.9, ink, 'Black', tracking=1.05)
        s.text(sub, w * 0.08, h * 0.03, h * 0.035, ink, 'Bold')
        s.text('$4.99', w * 0.92, h * 0.765, h * 0.03, ink, 'Bold', align='RIGHT')
    return draw


def draw_paper(s, w, h):
    s.rect(0, 0, w, h, 0xe8e2d2, rough=0.8)
    s.mid('NORTHSIDE HERALD', w / 2, h - 14, 12, w - 20, 0x1a1a1a, 'Black')
    s.rect(6, h - 24, w - 12, 1.0, 0x1a1a1a)
    s.text('SPORTS', 8, h - 40, 10, 0x8a1c1c, 'Black')
    s.mid('RIVERHAWKS DROP FOURTH STRAIGHT', w / 2, h - 58, 9.5, w - 16, 0x1a1a1a, 'Black')
    r = random.Random(4)
    s.rect(8, h - 128, w * 0.48, 60, 0x6a6a66, rough=0.8)
    for col_x in (w * 0.54, 8):
        y = h - 72 if col_x > 8 else h - 136
        while y > 10:
            s.rect(col_x, y, (w * 0.42 if col_x > 8 else w * 0.48) * r.uniform(0.7, 1.0), 1.6, 0x3a3a36, rough=0.8)
            y -= 4.2
    s.rect(w * 0.54, 10, w * 0.4, 1.0, 0x3a3a36)


def waiting():
    k = kit('nsbw', 'SM_NS_Barber_Waiting', ('body', 'print'), dust=0.1, low=0.12, print_px=1024)
    TEXTURE_SIZES.update({'nsbw_body': 2048, 'nsbw_print': 1024})
    mags = [('SPORT WEEK', 'THE PLAYOFF ISSUE', 0xf2efe6, 0xc4231c, 0x2a4a6a, 3), ('CLASSIC WHEELS', 'BARN FINDS: 12 CARS', 0x1a1a1a, 0xf2c230, 0x8a3a1c, 5),
            ("BARBER'S JOURNAL", 'FADES, THEN AND NOW', 0xe9e2cf, 0x1d3a2c, 0x4a6a5a, 7)]
    for i, (t, s_, g, ink, acc, sd) in enumerate(mags):
        k.panel(f'mag{i}', 0.21, 0.28, draw_mag(t, s_, g, ink, acc, sd))
    k.panel('paper', 0.30, 0.20, draw_paper)
    k.atlas.render()
    for cx in WAIT_X:
        M = Matrix.Translation((cx, WAIT_Y, 0.0)) @ Matrix.Rotation(math.radians(-90), 4, 'Z')
        chair_parts = []
        for s in (-1, 1):
            chair_parts.append((tube('frame', bend([(-0.19, s * 0.205, 0.0), (-0.19, s * 0.205, 0.39), (0.17, s * 0.205, 0.39), (0.19, s * 0.205, 0.0)], 0.05, 5),
                                     0.011, 12), 'chrome'))
            chair_parts.append((tube('upright', bend([(0.15, s * 0.19, 0.39), (0.19, s * 0.19, 0.56), (0.23, s * 0.19, 0.88)], 0.08, 5), 0.010, 12), 'chrome'))
            for fx in (-0.19, 0.19):
                chair_parts.append((lathe_solid('glide', [(0.0, 0.0), (0.014, 0.0), (0.013, 0.012), (0.0, 0.012)], Matrix.Translation((fx, s * 0.205, 0.0)), 12),
                                    'rubber', 0x141414))
        for bx_ in (-0.12, 0.12):
            chair_parts.append((tube('brace', [(bx_, -0.205, 0.39), (bx_, 0.205, 0.39)], 0.008, 10), 'chrome'))
        chair_parts.append((bx('seat_pan', -0.21, 0.19, -0.21, 0.21, 0.395, 0.41, 0.005), 'iron', 0xffffff))
        chair_parts.append((cushion('seat', -0.22, 0.20, -0.22, 0.22, 0.405, 0.45, 0.04, crown=0.01, sub=2), 'vinyl', OXBLOOD))
        rr = core.rounded_rect(-0.21, -0.21, 0.19, 0.21, 0.035, 4)
        chair_parts.append((tube('piping', [(x, y, 0.437) for x, y in rr], 0.004, 6, closed=True), 'vinyl', 0x3e1013))
        bp = cushion('back', -0.025, 0.025, -0.20, 0.20, -0.12, 0.12, 0.035, crown=0.0, sub=2,
                     matrix=Matrix.Translation((0.215, 0.0, 0.75)) @ Matrix.Rotation(math.radians(12), 4, 'Y'))
        chair_parts.append((bp, 'vinyl', OXBLOOD))
        for o, f, *col in chair_parts:
            o.data.transform(M)
            k.add(o, f, col[0] if col else 0xffffff)
        k.polish.append(((cx, WAIT_Y + 0.02, 0.45), 0.25, 0.8))
    # the side table between chairs 2 and 3, magazines and the sports page
    tx, ty = (WAIT_X[1] + WAIT_X[2]) / 2, -2.12
    for (dx, dy) in ((-0.19, -0.16), (0.19, -0.16), (-0.19, 0.16), (0.19, 0.16)):
        k.add(parts.rod('tleg', (tx + dx, ty + dy, 0.0), (tx + dx, ty + dy, 0.52), 0.011, 12), 'chrome')
    k.add(bx('ttop', tx - 0.22, tx + 0.22, ty - 0.19, ty + 0.19, 0.52, 0.545, 0.004), 'enamel', 0x7a1418)
    k.add(bx('tband', tx - 0.223, tx + 0.223, ty - 0.193, ty + 0.193, 0.518, 0.532, 0.003), 'chrome')
    k.add(bx('tshelf', tx - 0.20, tx + 0.20, ty - 0.17, ty + 0.17, 0.16, 0.175, 0.003), 'enamel', 0x7a1418)
    rnd = random.Random(9)
    for i in range(3):
        z = 0.545 + i * 0.004
        mm = Matrix.Translation((tx - 0.02 + i * 0.03, ty + 0.01 - i * 0.02, z)) @ Matrix.Rotation(math.radians(rnd.uniform(-25, 15)), 4, 'Z')
        o = bx('mag', -0.105, 0.105, -0.14, 0.14, 0.0, 0.0035, 0.001)
        o.data.transform(mm)
        k.add(o, 'paper', 0xe8e4da)
        k.decal(f'mag{i}', tuple(mm @ Vector((0, 0, 0.0036))), u=tuple(mm.to_3x3() @ Vector((1, 0, 0))), v=tuple(mm.to_3x3() @ Vector((0, 1, 0))),
                w=0.21, h=0.28, lift=0.0002)
    pm = Matrix.Translation((tx + 0.02, ty + 0.02, 0.175)) @ Matrix.Rotation(math.radians(8), 4, 'Z')
    for i in range(6):
        o = bx('stack', -0.105, 0.105, -0.14, 0.14, i * 0.004, i * 0.004 + 0.0035, 0.001)
        o.data.transform(pm @ Matrix.Rotation(math.radians(rnd.uniform(-6, 6)), 4, 'Z'))
        k.add(o, 'paper', [0xd8d2c4, 0xc4231c, 0x2a4a6a, 0xe8e4da, 0x1a1a1a, 0xf2c230][i])
    # the sports page, folded, dropped on chair 4's seat
    fm = Matrix.Translation((WAIT_X[3] + 0.02, WAIT_Y + 0.03, 0.452)) @ Matrix.Rotation(math.radians(-14), 4, 'Z')
    paper = bx('paper', -0.15, 0.15, -0.10, 0.10, 0.0, 0.006, 0.002)
    paper.data.transform(fm)
    k.add(paper, 'paper', 0xe0dac8)
    k.decal('paper', tuple(fm @ Vector((0, 0, 0.0062))), u=tuple(fm.to_3x3() @ Vector((1, 0, 0))), v=tuple(fm.to_3x3() @ Vector((0, 1, 0))), lift=0.0002)
    return [k.finish(35.0)]


# ------------------------------------------------------------------ the wall: photos, price board, TV, clock, coat rack

SOUTH = -2.40
BACK = 9.0


def grain(s, w, h, seed, n=180, col=0x000000, alpha=0.06):
    r = random.Random(seed)
    for _ in range(n):
        s.circle(r.uniform(0, w), r.uniform(0, h), r.uniform(0.002, 0.012) * min(w, h), col, alpha=alpha, n=8)


def draw_photo_dee(s, w, h):
    """Lonnie and young Dee by the chair, 1998: a faded color print in a white border, the date in the corner."""
    s.rect(0, 0, w, h, 0xf1ece0, rough=0.5)
    b = w * 0.05
    iw, ih = w - 2 * b, h - 2 * b - h * 0.08
    oy = b + h * 0.08
    s.rect(b, oy, iw, ih, 0xd9cdb0, rough=0.5)
    s.rect(b, oy, iw, ih * 0.38, 0x8fb8a8, rough=0.5)
    s.rect(b, oy + ih * 0.38, iw, ih * 0.02, 0x5a8a7a)
    for i in range(int(iw / (iw * 0.08))):
        s.rect(b + i * iw * 0.08, oy, iw * 0.04, ih * 0.06, 0x222222, alpha=0.8)
    # the chair
    s.rect(b + iw * 0.42, oy + ih * 0.1, iw * 0.2, ih * 0.22, 0x6a1a1a)
    s.rect(b + iw * 0.6, oy + ih * 0.1, iw * 0.05, ih * 0.45, 0x5a1414)
    # Lonnie: white smock, short grey-flecked hair
    s.rect(b + iw * 0.15, oy + ih * 0.0, iw * 0.2, ih * 0.62, 0xeeeae2)
    s.circle(b + iw * 0.25, oy + ih * 0.72, iw * 0.07, 0x7a5236)
    s.ring(b + iw * 0.25, oy + ih * 0.74, iw * 0.06, iw * 0.075, 0x2a2420, a0=0.2, a1=math.pi - 0.2)
    s.rect(b + iw * 0.21, oy + ih * 0.55, iw * 0.08, ih * 0.06, 0x1a1a1a)
    # Dee: a dark jacket, long hair
    s.rect(b + iw * 0.68, oy + ih * 0.0, iw * 0.18, ih * 0.55, 0x2a2a3a)
    s.circle(b + iw * 0.77, oy + ih * 0.64, iw * 0.06, 0xc89a78)
    s.rect(b + iw * 0.705, oy + ih * 0.48, iw * 0.13, ih * 0.2, 0x3a2418)
    s.circle(b + iw * 0.77, oy + ih * 0.68, iw * 0.065, 0x3a2418, alpha=0.6)
    grain(s, w, h, 3)
    s.rect(b, oy, iw, ih, 0xf0c070, alpha=0.18)
    s.text("Lonnie & Dee · '98", b + 2, b * 0.6, h * 0.05, 0x3a3a6a, 'Medium')
    s.text('98 6 14', b + iw - 2, oy + 4, h * 0.035, 0xff8a2a, 'Bold', align='RIGHT')


def draw_photo_street(s, w, h):
    """Fifth St in 1978: a black and white street of shopfronts, a car, the pole; sepia with age."""
    s.rect(0, 0, w, h, 0x1a1a1a, rough=0.5)
    m = w * 0.06
    iw, ih = w - 2 * m, h - 2 * m
    s.rect(m, m, iw, ih, 0xc9c0ae, rough=0.5)
    s.rect(m, m, iw, ih * 0.25, 0x6e6a62)
    r = random.Random(78)
    x = m
    while x < m + iw - 4:
        bw = r.uniform(iw * 0.12, iw * 0.22)
        bh = r.uniform(ih * 0.5, ih * 0.9)
        tone = r.choice([0x8a8478, 0x5e5a52, 0x9a9384, 0x4a4640])
        s.rect(x, m + ih * 0.25, min(bw, m + iw - x), min(bh, ih * 0.75), tone)
        for wy in range(3):
            for wx in range(3):
                s.rect(x + bw * (0.12 + wx * 0.3), m + ih * (0.42 + wy * 0.14), bw * 0.16, ih * 0.08, 0x2a2824)
        s.rect(x + bw * 0.05, m + ih * 0.25, bw * 0.9, ih * 0.12, 0x24221e)
        x += bw + 2
    s.rect(m + iw * 0.25, m + ih * 0.08, iw * 0.3, ih * 0.1, 0x222220)
    s.rect(m + iw * 0.29, m + ih * 0.17, iw * 0.2, ih * 0.06, 0x2a2826)
    for cx in (m + iw * 0.3, m + iw * 0.5):
        s.circle(cx, m + ih * 0.08, ih * 0.04, 0x0e0e0e)
    for i in range(6):
        s.rect(m + iw * 0.72, m + ih * (0.3 + i * 0.05), iw * 0.015, ih * 0.025, 0xeeeeee if i % 2 else 0x333333)
    grain(s, w, h, 9, 260, 0x000000, 0.08)
    s.rect(m, m, iw, ih, 0xb08040, alpha=0.22)
    s.text('FIFTH ST · 1978', m + iw - 3, m + 3, h * 0.04, 0xe8e2d2, 'Bold', align='RIGHT')


def draw_license_big(s, w, h):
    s.rect(0, 0, w, h, 0xf4efe2, rough=0.6)
    s.rect(6, 6, w - 12, h - 12, 0x1f3b6a, rough=0.6)
    s.rect(9, 9, w - 18, h - 18, 0xf4efe2, rough=0.6)
    s.mid('STATE BOARD OF', w / 2, h * 0.88, h * 0.035, w * 0.7, 0x1f3b6a, 'Bold', tracking=1.3)
    s.mid('BARBER EXAMINERS', w / 2, h * 0.83, h * 0.045, w * 0.8, 0x1f3b6a, 'Black', tracking=1.2)
    s.mid('MASTER BARBER', w / 2, h * 0.69, h * 0.07, w * 0.8, 0x1a1a1a, 'Black')
    s.mid('THIS CERTIFIES THAT', w / 2, h * 0.58, h * 0.025, w * 0.7, 0x444444, 'Bold')
    s.mid('LONNIE R. PRUITT', w / 2, h * 0.5, h * 0.055, w * 0.8, 0x1a1a1a, 'Bold')
    s.mid('IS LICENSED TO PRACTICE BARBERING', w / 2, h * 0.43, h * 0.022, w * 0.8, 0x444444, 'Bold')
    s.mid('LICENSE No. 04417 · EXPIRES 06/30/2027', w / 2, h * 0.36, h * 0.022, w * 0.8, 0x1a1a1a, 'Bold')
    s.circle(w * 0.28, h * 0.16, w * 0.09, 0xc9a24a, metal=0.7, rough=0.3)
    s.ring(w * 0.28, h * 0.16, w * 0.07, w * 0.075, 0x8a6a2a, metal=0.7, rough=0.3)
    s.rect(w * 0.55, h * 0.14, w * 0.3, 0.8, 0x1a1a1a)
    s.mid('SECRETARY', w * 0.7, h * 0.1, h * 0.02, w * 0.3, 0x444444, 'Bold')


def draw_styles(s, w, h):
    """A hairstyle chart: nine heads, numbered, in two colors on cream stock."""
    s.rect(0, 0, w, h, 0xece2c6, rough=0.75)
    s.rect(0, h - 90, w, 90, 0x8a1c1c, rough=0.7)
    s.mid('HAIR STYLES', w / 2, h - 45, 50, w * 0.8, 0xece2c6, 'Black', tracking=1.3)
    names = ['CREW', 'FLAT TOP', 'BUZZ', 'SIDE PART', 'POMP', 'FADE', 'IVY', 'CURLS', 'TAPER']
    hair = [0x1a1410, 0x2a1a10, 0x3a2a1a, 0x1c1410, 0x4a3018, 0x1a1a1a, 0x2a1c12, 0x120c08, 0x3a2416]
    for i in range(9):
        cx = w * (0.18 + 0.32 * (i % 3))
        cy = h - 190 - (i // 3) * (h - 140) / 3.2
        r = w * 0.09
        s.rect(cx - r * 0.55, cy - r * 1.9, r * 1.1, r * 0.9, 0xd8c8a8)
        s.circle(cx, cy, r, 0xc89a78)
        k_ = i % 5
        if names[i] == 'FLAT TOP':
            s.rect(cx - r * 0.95, cy + r * 0.3, r * 1.9, r * 0.9, hair[i])
        elif names[i] == 'POMP':
            s.circle(cx + r * 0.1, cy + r * 0.85, r * 0.8, hair[i])
        elif names[i] == 'CURLS':
            for a in range(9):
                s.circle(cx + r * 0.85 * math.cos(a * 0.4), cy + r * 0.5 + r * 0.5 * math.sin(a * 0.4), r * 0.35, hair[i])
        else:
            s.ring(cx, cy, r * (0.75 if names[i] != 'BUZZ' else 0.92), r * 1.08, hair[i], a0=0.1 + 0.1 * k_, a1=math.pi - 0.1)
        s.circle(cx, cy - r * 0.05, r * 0.88, 0xc89a78)
        s.circle(cx - r * 0.3, cy, r * 0.08, 0x2a1a10)
        s.circle(cx + r * 0.3, cy, r * 0.08, 0x2a1a10)
        s.mid(f'{i + 1}. {names[i]}', cx, cy - r * 2.25, 22, w * 0.3, 0x1a1a1a, 'Black')
    s.mid('ASK YOUR BARBER', w / 2, 26, 24, w * 0.8, 0x8a1c1c, 'Bold', tracking=1.3)


def draw_dial_face(s, w, h):
    s.circle(w / 2, h / 2, w / 2, 0xf4f1e8, rough=0.4)
    for i in range(60):
        a = math.pi / 2 - 2 * math.pi * i / 60
        r0 = w * (0.43 if i % 5 else 0.40)
        x0, y0 = w / 2 + r0 * math.cos(a), h / 2 + r0 * math.sin(a)
        x1, y1 = w / 2 + w * 0.46 * math.cos(a), h / 2 + w * 0.46 * math.sin(a)
        Sub.stroke(s, [(x0, y0), (x1, y1)], w * (0.012 if i % 5 == 0 else 0.004), 0x161616, 0.4)
    for i in range(1, 13):
        a = math.pi / 2 - 2 * math.pi * i / 12
        s.mid(str(i), w / 2 + w * 0.33 * math.cos(a), h / 2 + w * 0.33 * math.sin(a), w * 0.09, w * 0.14, 0x161616, 'Bold')
    s.mid('FIFTH ST. BARBERS', w / 2, h * 0.68, w * 0.035, w * 0.36, 0x7a1c1c, 'Black', tracking=1.2)
    s.mid('EST. 1995', w / 2, h * 0.3, w * 0.03, w * 0.2, 0x444444, 'Bold', tracking=1.3)


def draw_calendar(s, w, h):
    s.rect(0, 0, w, h, 0xf4f1e8, rough=0.7)
    s.rect(0, h * 0.5, w, h * 0.5, 0x2a3a4a, rough=0.6)
    s.rect(w * 0.1, h * 0.58, w * 0.8, h * 0.18, 0x8a1c1c)
    s.circle(w * 0.25, h * 0.58, w * 0.08, 0x111111)
    s.circle(w * 0.72, h * 0.58, w * 0.08, 0x111111)
    s.rect(w * 0.3, h * 0.76, w * 0.36, h * 0.07, 0x6a1414)
    s.mid('NORTHSIDE AUTO BODY · 555-0127', w / 2, h * 0.94, h * 0.03, w * 0.9, 0xf4f1e8, 'Black')
    s.mid('OCTOBER 2026', w / 2, h * 0.45, h * 0.04, w * 0.7, 0x1a1a1a, 'Black', tracking=1.2)
    days = ['S', 'M', 'T', 'W', 'T', 'F', 'S']
    for i, d in enumerate(days):
        s.mid(d, w * (0.1 + 0.8 * i / 6), h * 0.39, h * 0.022, w * 0.06, 0x8a1c1c, 'Bold')
    for n in range(1, 32):
        col_ = (n + 3) % 7
        row = (n + 3) // 7
        s.mid(str(n), w * (0.1 + 0.8 * col_ / 6), h * (0.34 - row * 0.062), h * 0.026, w * 0.07, 0x1a1a1a, 'Bold')
    s.rect(w * (0.1 + 0.8 * 1 / 6) - 8, h * (0.34 - 1 * 0.062) - 7, 16, 14, 0xc4231c, alpha=0.0)


def tv_image():
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0x8a5a2a, rough=0.3)
        s.rect(0, h * 0.55, w, h * 0.45, 0x1a1e2a)
        for i in range(40):
            r = random.Random(i)
            s.rect(r.uniform(0, w), h * 0.55 + r.uniform(0, h * 0.4), w * 0.01, h * 0.03, r.choice([0xc8a080, 0x3a3a5a, 0xe8e0d0, 0x6a2a2a]))
        s.ring(w * 0.5, h * 0.3, h * 0.12, h * 0.125, 0xf0e8d8)
        s.rect(w * 0.5 - 0.6, 0, 1.2, h * 0.55, 0xf0e8d8)
        s.rect(w * 0.08, h * 0.08, w * 0.12, h * 0.4, 0xc87a3a, alpha=0.8)
        for i, (x, y, c) in enumerate(((0.3, 0.3, 0x1f3b9a), (0.45, 0.22, 0xe8e8e8), (0.62, 0.35, 0x1f3b9a), (0.55, 0.15, 0xe8e8e8), (0.38, 0.4, 0xe8e8e8))):
            s.rect(w * x, h * y, w * 0.025, h * 0.14, c)
            s.circle(w * x + w * 0.0125, h * y + h * 0.17, h * 0.025, 0x6a4a30)
        s.rect(w * 0.05, h * 0.86, w * 0.5, h * 0.09, 0x111418)
        s.text('NSX 54', w * 0.07, h * 0.885, h * 0.05, 0xf2f2f2, 'Black')
        s.text('RVR 61', w * 0.22, h * 0.885, h * 0.05, 0xf2c230, 'Black')
        s.text('3RD  4:12', w * 0.38, h * 0.885, h * 0.05, 0xf2f2f2, 'Bold')
        s.text('NSN', w * 0.9, h * 0.08, h * 0.05, 0xf2f2f2, 'Black', align='RIGHT')
    return render_image('ns_screen_tv', 160.0, 90.0, 1024, draw)


def prices_image():
    def draw(s, w, h):
        s.rect(0, 0, w, h, 0x141414, rough=0.6)
        s.mid('PRICES', w / 2, h * 0.92, h * 0.05, w * 0.7, 0xe8e2d0, 'Black', tracking=1.4)
        items = [('LINE-UP', '$12'), ('BUZZ CUT', '$15'), ('CREW CUT', '$22'), ('CROP · SIDE PART', '$24'), ('UNDERCUT', '$26'), ('CURLS', '$26'),
                 ('AFRO SHAPE-UP', '$28'), ('BOB', '$30'), ('TRIM · PONYTAIL', '$30'), ('BRAIDS', '$65'), ('HOT TOWEL SHAVE', '$25'), ('BEARD TRIM', '$10+'),
                 ('COLOR', '$45'), ('BROWS', '$8')]
        for i, (a, b) in enumerate(items):
            y = h * 0.84 - i * h * 0.056
            s.text(a, w * 0.08, y, h * 0.03, 0xe8e2d0, 'Bold')
            s.text(b, w * 0.92, y, h * 0.03, 0xe8e2d0, 'Bold', align='RIGHT')
        s.mid('CUT & BEARD: $5 OFF', w / 2, h * 0.04, h * 0.026, w * 0.8, 0xc9a24a, 'Bold')
    return render_image('ns_screen_prices', 300.0, 400.0, 768, draw)


def coat(k, hx, hz, wall_y, width, length, col, seed, fin='cloth', buttons=0x1a1612):
    """A coat hung by its collar loop on a hook: tented from the hook to the shoulders, the back flat on the wall,
    vertical folds deepening toward the hem, a collar, a button stand, pocket flaps and the sleeves hanging at the
    sides."""
    rnd = random.Random(seed)
    nz, na = 30, 40
    rings, front = [], []
    ph = [rnd.uniform(0.0, 6.28) for _ in range(3)]
    for j in range(nz):
        t = j / (nz - 1)
        z = hz - 0.035 - t * length
        # the shoulders: a smooth ease from the hook's loop out to the full width (no kink where they meet the body)
        s = min(1.0, t / 0.2)
        hw = 0.045 + (width / 2 - 0.045) * (1.0 - (1.0 - s) ** 2) + 0.035 * max(0.0, t - 0.2)
        dep = 0.035 + 0.085 * (1.0 - (1.0 - s) ** 2) - 0.035 * t
        ring = []
        for i in range(na):
            a = 2 * math.pi * i / na
            ca, sa = math.cos(a), math.sin(a)
            # soft vertical folds, deepening toward the hem; the back stays against the wall
            grow = min(1.0, t * 2.5)
            fold = (0.010 * math.sin(a * 5 + ph[0] + t * 1.5) + 0.005 * math.sin(a * 9 + ph[1] - t * 2.0)) * grow
            f = 0.5 + 0.5 * sa
            f = f * f * (3.0 - 2.0 * f)
            x = hx + (hw + fold * 0.35) * ca
            y = wall_y + 0.004 + (dep + fold * f) * f
            ring.append((x, y, z))
        rings.append(ring)
        front.append(max(p[1] for p in ring))
    body = core.loft('coat', rings)
    core.subdivide(body, 1)
    body.data.shade_smooth()
    k.add(body, fin, col, soft=True)
    # the collar: a rolled band round the top
    zc = hz - 0.035 - 0.07 * length
    jc = int(0.07 * (nz - 1))
    k.add(tube('collar', core.catmull_rom([(p[0], p[1] + 0.006, zc) for p in rings[jc]] + [rings[jc][0]], 2), 0.012, 10, closed=False), fin, col,
          soft=True)
    # the button stand and buttons, two pocket flaps
    for b in range(4):
        tz = 0.30 + b * 0.13
        jj = int(tz * (nz - 1))
        zz = hz - 0.035 - tz * length
        k.add(parts.rod('button', (hx + 0.02, front[jj] - 0.002, zz), (hx + 0.02, front[jj] + 0.006, zz), 0.011, 12), 'plastic', buttons)
    for sx in (-1, 1):
        tz = 0.62
        jj = int(tz * (nz - 1))
        zz = hz - 0.035 - tz * length
        flap = bx('flap', hx + sx * 0.13 - 0.07, hx + sx * 0.13 + 0.07, front[jj] - 0.012, front[jj] + 0.004, zz - 0.045, zz, 0.004)
        k.add(flap, fin, col)
    # the sleeves hang down the sides, a little forward, cuffs turned in
    for sx in (-1, 1):
        p0 = (hx + sx * (width / 2 - 0.05), wall_y + 0.07, hz - 0.12)
        p1 = (hx + sx * (width / 2 - 0.01), wall_y + 0.10, hz - 0.38)
        p2 = (hx + sx * (width / 2 - 0.04), wall_y + 0.12, hz - 0.66)
        pts = core.catmull_rom([p0, p1, p2], 10)
        rr = []
        for q, pnt in enumerate(pts):
            t = q / (len(pts) - 1)
            r_ = 0.058 - 0.014 * t
            d = (Vector(pts[min(q + 1, len(pts) - 1)]) - Vector(pts[max(q - 1, 0)])).normalized()
            u = d.cross(Vector((0, 1, 0))).normalized()
            v = d.cross(u).normalized()
            rr.append([tuple(Vector(pnt) + u * r_ * math.cos(a) * 1.0 + v * r_ * math.sin(a) * 0.8 + u * 0.003 * math.sin(a * 4 + q * 0.5)) for a in
                       [2 * math.pi * i / 16 for i in range(16)]])
        sl = core.loft('sleeve', rr)
        core.subdivide(sl, 1)
        sl.data.shade_smooth()
        k.add(sl, fin, col, soft=True)


def flat_cap(k, at, col):
    """A flat cap hung on a hook by its crown, the top facing out from the wall and the brim down."""
    M = Matrix.Translation(at) @ Matrix.Rotation(math.radians(-90), 4, 'X')
    crown = bx('cap_crown', -0.13, 0.13, -0.11, 0.09, 0.0, 0.065, 0.03, 1)
    core.subdivide(crown, 2)
    for v in crown.data.vertices:
        if v.co.z > 0.03:
            v.co.y += 0.03 * (v.co.z / 0.065)
            v.co.z -= 0.012 * (v.co.y / 0.1) ** 2
    crown.data.transform(M)
    crown.data.shade_smooth()
    k.add(crown, 'felt', col, soft=True)
    brim = bx('cap_brim', -0.09, 0.09, 0.07, 0.14, 0.0, 0.007, 0.006, 1)
    core.subdivide(brim, 1)
    for v in brim.data.vertices:
        v.co.z -= 0.01 * (v.co.x / 0.09) ** 2
    brim.data.transform(M)
    k.add(brim, 'felt', col)
    k.add(lathe_solid('cap_button', [(0.0, 0.0), (0.008, 0.0), (0.006, 0.006), (0.0, 0.007)], M @ Matrix.Translation((0.0, -0.01, 0.062)), 12), 'felt', col)


def wall():
    k = kit('nsbw2', 'SM_NS_Barber_Wall', ('body', 'print', 'ns_screen_tv', 'ns_screen_prices'), dust=0.15, low=0.05, print_px=2048)
    TEXTURE_SIZES.update({'nsbw2_body': 2048, 'nsbw2_print': 2048})
    screen_material('ns_screen_tv', tv_image(), strength=2.2)
    screen_material('ns_screen_prices', prices_image(), lit=False, rough=0.5)
    k.panel('dee', 0.30, 0.24, draw_photo_dee, density=1.6)
    k.panel('street', 0.54, 0.34, draw_photo_street, density=1.3)
    k.panel('lic', 0.26, 0.34, draw_license_big, density=1.4)
    k.panel('styles', 0.56, 0.76, draw_styles)
    k.panel('dial', 0.34, 0.34, draw_dial_face, density=1.2)
    k.panel('cal', 0.30, 0.46, draw_calendar, density=1.2)
    k.atlas.render()
    U, V = (-1, 0, 0), (0, 0, 1)          # print axes on the south wall (read from inside, facing +y)
    wy = SOUTH

    def frame(x0, x1, z0, z1, key, depth=0.025, mould=0.03, col=0x2a1a10, mat=True):
        """A picture frame on the south wall: moulding, a mat, the print under glass (baked)."""
        for (a, b, c_, d) in ((x0, x1, z0, z0 + mould), (x0, x1, z1 - mould, z1), (x0, x0 + mould, z0, z1), (x1 - mould, x1, z0, z1)):
            k.add(bx('frame', a, b, wy, wy + depth, c_, d, 0.004, 2), 'wood', col, aux=0)
        k.add(bx('frame_back', x0 + mould, x1 - mould, wy, wy + depth - 0.012, z0 + mould, z1 - mould), 'paper', 0xf0ece2 if mat else 0x111111)
        k.decal(key, ((x0 + x1) / 2, wy + depth - 0.012, (z0 + z1) / 2), u=U, v=V, w=(x1 - x0) - 2 * mould - (0.04 if mat else 0.0),
                h=(z1 - z0) - 2 * mould - (0.04 if mat else 0.0), lift=0.0004)
        k.add(bx('frame_hook', (x0 + x1) / 2 - 0.004, (x0 + x1) / 2 + 0.004, wy, wy + 0.004, z1 + 0.02, z1 + 0.05), 'brass')

    frame(1.30, 1.72, 1.72, 2.06, 'dee', col=0x1a1a1a)
    frame(2.98, 3.64, 1.62, 2.06, 'street', col=0x3a2414)
    frame(3.92, 4.26, 1.62, 2.06, 'lic', col=0xb08d4a)
    # the price board (core-drawn screen) in a walnut frame
    px0, px1, pz0, pz1 = 2.35, 2.80, 1.50, 2.10
    for (a, b, c_, d) in ((px0 - 0.04, px1 + 0.04, pz0 - 0.04, pz0), (px0 - 0.04, px1 + 0.04, pz1, pz1 + 0.04), (px0 - 0.04, px0, pz0, pz1), (px1, px1 + 0.04, pz0, pz1)):
        k.add(bx('pframe', a, b, wy, wy + 0.035, c_, d, 0.005, 2), 'wood', WALNUT, aux=0)
    k.add(bx('pback', px0, px1, wy, wy + 0.02, pz0, pz1), 'wood', 0x1a1a1a)
    scr = quad('prices', [(px1, wy + 0.021, pz0), (px0, wy + 0.021, pz0), (px0, wy + 0.021, pz1), (px1, wy + 0.021, pz1)])
    uv = scr.data.uv_layers.new(name='UVMap')
    for li, t in enumerate(((0, 0), (1, 0), (1, 1), (0, 1))):
        uv.data[li].uv = t
    core.orient_normals(scr, (0, 1, 0))
    k.special(scr, 'ns_screen_prices')
    # the hairstyle chart, tacked up, a corner lifting
    sx0, sx1, sz0, sz1 = 5.10, 5.66, 1.42, 2.18
    k.decal('styles', ((sx0 + sx1) / 2, wy + 0.0015, (sz0 + sz1) / 2), u=U, v=V, w=sx1 - sx0, h=sz1 - sz0, lift=0.0, nu=6, nv=6,
            disp=lambda s_, t: 0.012 * max(0.0, s_ - 0.75) * max(0.0, t - 0.7) * 30.0 + 0.001)
    k.add(bx('styles_back', sx0 + 0.002, sx1 - 0.002, wy, wy + 0.0012, sz0 + 0.002, sz1 - 0.002), 'paper', 0xd8d0b8)
    for (tx_, tz_) in ((sx0 + 0.02, sz1 - 0.02), (sx1 - 0.02, sz0 + 0.02), (sx0 + 0.02, sz0 + 0.02)):
        k.add(parts.rod('tack', (tx_, wy + 0.001, tz_), (tx_, wy + 0.006, tz_), 0.006, 12), 'enamel', 0xc4231c)
    # the calendar near the back
    cm = Matrix.Translation((8.12, wy + 0.004, 1.55))
    k.decal('cal', (8.12, wy + 0.0035, 1.55), u=U, v=V, w=0.30, h=0.46, lift=0.0, nu=1, nv=4, disp=lambda s_, t: 0.006 * (1 - t))
    k.add(bx('cal_back', 7.975, 8.265, wy, wy + 0.003, 1.33, 1.77), 'paper', 0xe0dace)
    k.add(parts.rod('cal_nail', (8.12, wy, 1.79), (8.12, wy + 0.012, 1.79), 0.002, 6), 'iron')
    # the coat rack: a walnut rail with five brass double hooks; two coats, a cap, the spare cape
    rx0, rx1, rz = 6.30, 7.50, 1.72
    k.add(bx('rack', rx0, rx1, wy, wy + 0.022, rz - 0.05, rz + 0.05, 0.006, 2), 'wood', WALNUT, aux=0)
    hooks = [rx0 + 0.1 + i * 0.25 for i in range(5)]
    for hxp in hooks:
        k.add(tube('hook_lo', bend([(hxp, wy + 0.022, rz), (hxp, wy + 0.07, rz - 0.01), (hxp, wy + 0.09, rz + 0.03)], 0.02, 4), 0.005, 8), 'brass')
        k.add(tube('hook_hi', bend([(hxp, wy + 0.022, rz + 0.02), (hxp, wy + 0.045, rz + 0.05), (hxp, wy + 0.05, rz + 0.08)], 0.015, 4), 0.004, 8), 'brass')
        k.add(parts.rod('hook_base', (hxp, wy + 0.022, rz + 0.01), (hxp, wy + 0.028, rz + 0.01), 0.014, 12), 'brass')
    coat(k, hooks[0], rz + 0.0, wy + 0.025, 0.46, 0.78, 0x6a4a2a, 3, 'cloth', 0x3a2414)
    coat(k, hooks[2], rz + 0.0, wy + 0.025, 0.50, 0.98, 0x1f2026, 5, 'cloth')
    # the spare cape, black nylon, folded over hook 5
    cape = []
    for j, z in enumerate(np.linspace(rz + 0.02, rz - 0.62, 10)):
        t = j / 9
        hw_ = 0.04 + 0.22 * min(1.0, t * 4)
        cape.append([(hooks[4] + hw_ * math.cos(a) + 0.01 * math.sin(a * 7 + j), wy + 0.035 + (0.02 + 0.03 * t) * (0.5 + 0.5 * math.sin(a)), z)
                     for a in [2 * math.pi * i / 20 for i in range(20)]])
    spare = core.loft('cape', cape)
    core.subdivide(spare, 1)
    k.add(spare, 'nylon', 0x18181c, soft=True)
    # a flat cap on hook 2
    flat_cap(k, (hooks[1], wy + 0.06, rz + 0.02), 0x5a5248)
    # the TV on its bracket, high on the back wall, turned toward the chairs
    tvc = Vector((8.78, 1.42, 2.40))
    tvm = Matrix.Translation(tvc) @ Matrix.Rotation(math.radians(18), 4, 'Z') @ Matrix.Rotation(math.radians(-8), 4, 'Y')
    tw, th = 0.89, 0.51
    for o, f, c in ((bx('tv_body', 0.0, 0.055, -tw / 2 - 0.012, tw / 2 + 0.012, -th / 2 - 0.012, th / 2 + 0.02, 0.006, 2), 'plastic', 0x111214),
                    (bx('tv_back', 0.055, 0.075, -0.2, 0.2, -0.15, 0.15, 0.01), 'plastic', 0x141517),
                    (bx('tv_logo', -0.001, 0.0, -0.03, 0.03, -th / 2 - 0.009, -th / 2 - 0.003), 'chrome', 0xffffff)):
        o.data.transform(tvm)
        k.add(o, f, c)
    sc_ = quad('tv_screen', [(-0.0005, tw / 2, -th / 2), (-0.0005, -tw / 2, -th / 2), (-0.0005, -tw / 2, th / 2), (-0.0005, tw / 2, th / 2)])
    uv = sc_.data.uv_layers.new(name='UVMap')
    for li, t in enumerate(((0, 0), (1, 0), (1, 1), (0, 1))):
        uv.data[li].uv = t
    sc_.data.transform(tvm)
    core.orient_normals(sc_, tuple(tvm.to_3x3() @ Vector((-1, 0, 0))))
    k.special(sc_, 'ns_screen_tv')
    arm_a = tvm @ Vector((0.075, 0.0, 0.0))
    k.add(bx('tv_wallplate', BACK - 0.012, BACK, 1.30, 1.54, 2.25, 2.55, 0.004), 'iron')
    k.add(tube('tv_arm', bend([(BACK - 0.012, 1.42, 2.40), (BACK - 0.10, 1.42, 2.40), tuple(arm_a)], 0.04, 4), 0.018, 12), 'iron')
    k.add(tube('tv_cable', bend([tuple(tvm @ Vector((0.06, 0.1, -0.15))), (BACK - 0.01, 1.62, 2.2), (BACK - 0.01, 1.62, 1.2), (BACK - 0.01, 1.70, 0.35)], 0.06, 5),
               0.004, 6), 'rubber', 0x151515)
    k.add(bx('tv_outlet', BACK - 0.007, BACK, 1.66, 1.74, 0.30, 0.42, 0.003), 'plastic', 0xeee9dc)
    # the clock over the middle of the back wall (dial baked; the game adds the hands at its middle)
    cc = Vector((BACK - 0.06, 0.03, 2.62))
    rot = Matrix.Translation(cc) @ Matrix.Rotation(math.radians(90), 4, 'Y')
    k.add(lathe_solid('clock_body', [(0.0, -0.06), (0.19, -0.06), (0.192, -0.05), (0.19, 0.0), (0.18, 0.012), (0.172, 0.006), (0.0, 0.006)],
                      Matrix.Translation(cc) @ Matrix.Rotation(math.radians(-90), 4, 'Y'), 64), 'plastic', 0x161616)
    k.decal('dial', (cc.x - 0.0065, cc.y, cc.z), u=(0, -1, 0), v=(0, 0, 1), w=0.34, h=0.34, lift=0.0,
            outline=[(0.5 + 0.5 * math.cos(2 * math.pi * q / 48), 0.5 + 0.5 * math.sin(2 * math.pi * q / 48)) for q in range(48)])
    k.add(parts.rod('clock_pin', (cc.x - 0.0065, cc.y, cc.z), (cc.x - 0.012, cc.y, cc.z), 0.004, 12), 'iron')
    return [k.finish(35.0)]


# ------------------------------------------------------------------ the lights

PENDANT_Z = 2.46


def lights():
    k = kit('nsbl', 'SM_NS_Barber_Lights', ('body',), dust=0.4, low=0.0)
    TEXTURE_SIZES.update({'nsbl_body': 1024})
    warm, cool = [], []
    for x in CHAIR_X:
        c = (x, 0.75)
        M = Matrix.Translation((c[0], c[1], 0.0))
        k.add(lathe_solid('canopy', [(0.0, CEIL - 0.03), (0.065, CEIL - 0.03), (0.07, CEIL - 0.02), (0.07, CEIL), (0.0, CEIL)], M, 32), 'brass')
        k.add(parts.rod('stem', (c[0], c[1], CEIL - 0.03), (c[0], c[1], PENDANT_Z + 0.20), 0.007, 12), 'brass')
        k.add(lathe_solid('fitter', [(0.0, PENDANT_Z + 0.20), (0.02, PENDANT_Z + 0.20), (0.05, PENDANT_Z + 0.17), (0.06, PENDANT_Z + 0.12),
                                     (0.058, PENDANT_Z + 0.11), (0.0, PENDANT_Z + 0.11)], M, 32), 'brass')
        k.add(lathe_solid('fitter_ring', [(0.0, PENDANT_Z + 0.10), (0.063, PENDANT_Z + 0.10), (0.063, PENDANT_Z + 0.112), (0.0, PENDANT_Z + 0.112)], M, 32), 'brass')
        for a in (0.0, 2.1, 4.2):
            k.add(parts.rod('screw', (c[0] + 0.063 * math.cos(a), c[1] + 0.063 * math.sin(a), PENDANT_Z + 0.106),
                            (c[0] + 0.072 * math.cos(a), c[1] + 0.072 * math.sin(a), PENDANT_Z + 0.106), 0.004, 8), 'brass')
        warm.append(lathe_solid('globe', [(0.0, PENDANT_Z - 0.16), (0.04, PENDANT_Z - 0.155), (0.09, PENDANT_Z - 0.12), (0.125, PENDANT_Z - 0.05),
                                          (0.13, PENDANT_Z), (0.12, PENDANT_Z + 0.05), (0.09, PENDANT_Z + 0.09), (0.058, PENDANT_Z + 0.105), (0.0, PENDANT_Z + 0.105)],
                                M, 48))
    for (x0, x1) in ((CHAIR_X[0] - 0.61, CHAIR_X[0] + 0.61), (CHAIR_X[1] - 0.61, CHAIR_X[1] + 0.61), (7.30, 8.52)):
        y = 1.90
        k.add(bx('housing', x0, x1, y - 0.11, y + 0.11, CEIL - 0.05, CEIL, 0.006), 'enamel', 0xf0eee6)
        for xe in (x0, x1):
            k.add(bx('endcap', xe - 0.012, xe + 0.012, y - 0.135, y + 0.135, CEIL - 0.10, CEIL - 0.002, 0.008), 'plastic', 0xe8e6de)
        prof = [(0.0, -0.13, CEIL - 0.05)] + [(0.0, 0.13 * math.cos(a), CEIL - 0.05 - 0.05 * math.sin(a)) for a in np.linspace(math.pi, 0.0, 12)]
        bm = bmesh.new()
        r0 = [bm.verts.new((x0 + 0.012, p[1] + y, p[2])) for p in prof[1:]]
        r1 = [bm.verts.new((x1 - 0.012, p[1] + y, p[2])) for p in prof[1:]]
        for i in range(len(r0) - 1):
            bm.faces.new((r0[i], r0[i + 1], r1[i + 1], r1[i]))
        bm.faces.new(list(reversed(r0)))
        bm.faces.new(r1)
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        cool.append(core.mesh_object('lens', bm))
    obj = k.finish(35.0)
    return [obj, glow(warm, 'SM_NS_Barber_Lights_GlowWarm', ANCHOR, LOG), glow(cool, 'SM_NS_Barber_Lights_GlowCool', ANCHOR, LOG)]


# ------------------------------------------------------------------ the shampoo sink (P1) and the cape (P1)

SINK_X = 7.90


def sink():
    """SM_NS_Barber_Sink (SITE): a walnut cabinet with a black porcelain shampoo bowl (its neck notch to the front), a
    gooseneck and sprayer; the reclined shampoo chair in front of it, facing south."""
    k = kit('nsbk', 'SM_NS_Barber_Sink', ('body',), dust=0.1, low=0.12)
    TEXTURE_SIZES.update({'nsbk_body': 2048})
    x0, x1, y0, y1 = SINK_X - 0.60, SINK_X + 0.60, 1.95, 2.45
    k.add(bx('toe', x0 + 0.03, x1 - 0.03, y0 + 0.07, y1, 0.0, 0.10), 'enamel', 0x161616)
    k.add(bx('cab', x0, x1, y0, y1, 0.10, 0.85, 0.004), 'wood', WALNUT, aux=0)
    for (a, b) in ((x0 + 0.04, SINK_X - 0.02), (SINK_X + 0.02, x1 - 0.04)):
        k.add(bx('door', a, b, y0 - 0.02, y0, 0.13, 0.82, 0.004), 'wood', WALNUT, aux=2)
        k.add(bx('panel', a + 0.07, b - 0.07, y0 - 0.03, y0 - 0.02, 0.21, 0.74, 0.006), 'wood', WALNUT, aux=2)
    k.add(bx('top', x0 - 0.02, x1 + 0.02, y0 - 0.05, y1, 0.85, 0.88, 0.006), 'marble', MARBLE)
    # the bowl: a thick porcelain shell, tilted toward the chair, its front rim notched for the neck
    prof = [(0.0, -0.17), (0.09, -0.17), (0.20, -0.12), (0.27, -0.03), (0.285, 0.0), (0.27, 0.012), (0.24, 0.0), (0.18, -0.08), (0.085, -0.145),
            (0.0, -0.15)]
    bowl = core.lathe('bowl', prof, segments=56)
    weld(bowl)
    core.orient_normals(bowl)
    for v in bowl.data.vertices:
        a = math.atan2(v.co.y, v.co.x)
        d = abs(((a + math.pi / 2) + math.pi) % (2 * math.pi) - math.pi)
        if v.co.z > -0.06 and d < 0.6:
            v.co.z -= 0.075 * math.cos(d / 0.6 * math.pi / 2) ** 2 * min(1.0, (v.co.z + 0.06) / 0.07)
    bowl.data.transform(Matrix.Translation((SINK_X, 2.05, 0.98)) @ Matrix.Rotation(math.radians(8), 4, 'X'))
    bowl.data.shade_smooth()
    k.add(bowl, 'porcelain', 0x161616)
    k.add(lathe_solid('drain', [(0.0, -0.152), (0.03, -0.152), (0.03, -0.148), (0.0, -0.148)],
                      Matrix.Translation((SINK_X, 2.05, 0.98)) @ Matrix.Rotation(math.radians(8), 4, 'X'), 24), 'chrome')
    k.add(bx('pedestal', SINK_X - 0.16, SINK_X + 0.16, 1.98, 2.25, 0.86, 0.90, 0.02), 'porcelain', 0x161616)
    k.add(tube('faucet', bend([(SINK_X - 0.05, 2.38, 0.88), (SINK_X - 0.05, 2.38, 1.12), (SINK_X - 0.05, 2.22, 1.12), (SINK_X - 0.05, 2.20, 1.06)], 0.05, 6),
               0.013, 12), 'chrome')
    for dx in (-0.16, 0.06):
        k.add(lathe_solid('handle', [(0.0, 0.0), (0.02, 0.0), (0.025, 0.02), (0.012, 0.05), (0.0, 0.055)], Matrix.Translation((SINK_X + dx, 2.38, 0.88)), 16), 'chrome')
    k.add(lathe_solid('sprayer', [(0.0, 0.0), (0.017, 0.0), (0.02, 0.03), (0.016, 0.12), (0.0, 0.125)], Matrix.Translation((SINK_X + 0.22, 2.36, 0.88)), 20), 'chrome')
    k.add(tube('hose', core.catmull_rom([(SINK_X + 0.22, 2.36, 0.88), (SINK_X + 0.27, 2.30, 0.84), (SINK_X + 0.30, 2.20, 0.86)], 5), 0.007, 8), 'rubber', 0x1a1a1a)
    # the reclined shampoo chair, facing south (-y), its back up toward the bowl
    cy = 1.30
    k.add(lathe_solid('base', [(0.0, 0.0), (0.24, 0.0), (0.22, 0.03), (0.07, 0.07), (0.0, 0.08)], Matrix.Translation((SINK_X, cy, 0.0)), 48), 'chrome')
    k.add(parts.rod('column', (SINK_X, cy, 0.05), (SINK_X, cy, 0.36), 0.05, 24), 'chrome')
    k.add(bx('seat_box', SINK_X - 0.24, SINK_X + 0.24, cy - 0.22, cy + 0.24, 0.34, 0.42, 0.03, 2), 'enamel', 0x161616)
    k.add(cushion('seat', SINK_X - 0.25, SINK_X + 0.25, cy - 0.26, cy + 0.24, 0.40, 0.48, 0.05, crown=0.012), 'vinyl', 0x1c1a1a)
    bm_ = Matrix.Translation((SINK_X, cy + 0.38, 0.72)) @ Matrix.Rotation(math.radians(-38), 4, 'X')
    k.add(cushion('back', -0.24, 0.24, -0.05, 0.05, -0.32, 0.32, 0.05, crown=0.0, matrix=bm_), 'vinyl', 0x1c1a1a)
    for s in (-1, 1):
        k.add(tube('arm', bend([(SINK_X + s * 0.29, cy + 0.20, 0.42), (SINK_X + s * 0.29, cy + 0.20, 0.64), (SINK_X + s * 0.29, cy - 0.22, 0.64),
                                (SINK_X + s * 0.29, cy - 0.22, 0.42)], 0.05, 5), 0.013, 10), 'chrome')
        k.add(cushion('arm_pad', SINK_X + s * 0.29 - 0.04, SINK_X + s * 0.29 + 0.04, cy - 0.20, cy + 0.18, 0.645, 0.685, 0.02, crown=0.004), 'vinyl', 0x1c1a1a)
    k.add(tube('foot_arm', bend([(SINK_X, cy - 0.20, 0.36), (SINK_X, cy - 0.45, 0.25), (SINK_X, cy - 0.58, 0.22)], 0.05, 4), 0.014, 10), 'chrome')
    k.add(bx('foot', SINK_X - 0.18, SINK_X + 0.18, cy - 0.70, cy - 0.50, 0.19, 0.22, 0.008), 'chrome')
    k.polish = [((SINK_X, cy, 0.48), 0.2, 0.8), ((SINK_X, cy + 0.42, 0.8), 0.18, 0.6)]
    k.grime = [((SINK_X, 2.05, 0.9), 0.3, 0.5)]
    return [k.finish(35.0)]


def cape():
    """SM_NS_Barber_Cape (PROP): the cutting cape draped (cloth simulated) on a seated "Average" build in the barber chair;
    origin at the neck's middle at the collar. Built in the chair's frame, then moved to its origin."""
    neck = Vector((0.125, 0.0, 1.15))
    # the stand-in body: torso, hips, thighs, knees, shins, arms on the armrests (chair frame, the sitter facing -X)
    body = []
    tilt = Matrix.Rotation(math.radians(12.0), 4, 'Y')
    torso = bx('torso', -0.11, 0.11, -0.19, 0.19, -0.30, 0.26, 0.09, 3)
    torso.data.transform(Matrix.Translation((0.17, 0.0, 0.88)) @ tilt)
    body.append(torso)
    body.append(bx('hips', -0.05, 0.22, -0.19, 0.19, 0.55, 0.72, 0.08, 3))
    for s in (-1, 1):
        body.append(parts.rod('thigh', (0.05, s * 0.10, 0.65), (-0.40, s * 0.11, 0.66), 0.085, 16))
        body.append(parts.rod('shin', (-0.42, s * 0.11, 0.62), (-0.50, s * 0.12, 0.24), 0.06, 16))
        body.append(parts.rod('upper_arm', (0.16, s * 0.22, 1.08), (0.04, s * 0.28, 0.84), 0.05, 12))
        body.append(parts.rod('forearm', (0.04, s * 0.28, 0.84), (-0.24, s * 0.27, 0.82), 0.042, 12))
    body.append(parts.rod('neck', tuple(neck - Vector((0.0, 0.0, 0.05))), tuple(neck + Vector((0.02, 0.0, 0.10))), 0.06, 16))
    for b in body:
        core.orient_normals(b)
    # the cape: a sheet round the neck, long in front, short behind, pinned at its collar ring
    bm = bmesh.new()
    nr, na = 14, 48
    rings = []
    for j in range(nr + 1):
        t = j / nr
        ring = []
        for i in range(na):
            a = 2 * math.pi * i / na
            front = (1.0 - math.cos(a - math.pi)) / 2.0
            Rr = 0.075 + (0.13 + 0.66 * front ** 0.8) * t
            ring.append(bm.verts.new((neck.x + Rr * math.cos(a), neck.y + Rr * math.sin(a) * 1.05, neck.z + 0.01 - 0.02 * t)))
        rings.append(ring)
    for j in range(nr):
        for i in range(na):
            bm.faces.new((rings[j][i], rings[j][(i + 1) % na], rings[j + 1][(i + 1) % na], rings[j + 1][i]))
    sheet = core.mesh_object('cape_sheet', bm)
    vg = sheet.vertex_groups.new(name='pin')
    vg.add(list(range(na)), 1.0, 'REPLACE')
    cols = []
    for c in body:
        c.modifiers.new('col', 'COLLISION')
        c.collision.thickness_outer = 0.006
        c.collision.cloth_friction = 8.0
        cols.append(c)
    # the chair as a few boxes (its arms, seat front, footrest): cheap to collide with
    for (a, b, c_, d, e, f) in ((-0.31, 0.21, 0.255, 0.345, 0.70, 0.80), (-0.31, 0.21, -0.345, -0.255, 0.70, 0.80), (-0.27, -0.21, -0.20, 0.20, 0.22, 0.44),
                                (-0.66, -0.36, -0.21, 0.21, 0.16, 0.24)):
        cb = bx('chair_col', a, b, c_, d, e, f)
        cb.modifiers.new('col', 'COLLISION')
        cb.collision.thickness_outer = 0.006
        cols.append(cb)
    mod = sheet.modifiers.new('cloth', 'CLOTH')
    st = mod.settings
    st.quality = 5
    st.mass = 0.15
    st.tension_stiffness = 25.0
    st.compression_stiffness = 25.0
    st.shear_stiffness = 10.0
    st.bending_stiffness = 0.15
    st.air_damping = 2.0
    st.vertex_group_mass = 'pin'
    cs = mod.collision_settings
    cs.collision_quality = 3
    cs.distance_min = 0.006
    cs.use_self_collision = False
    frames = 55
    mod.point_cache.frame_start = 1
    mod.point_cache.frame_end = frames
    sc = bpy.context.scene
    sc.frame_start, sc.frame_end = 1, frames
    bpy.context.view_layer.update()
    for f in range(1, frames + 1):
        sc.frame_set(f)
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(sheet.evaluated_get(dg))
    sc.frame_set(1)
    for o in cols + [sheet]:
        bpy.data.objects.remove(o)
    drape = core.link(bpy.data.objects.new('cape', me))
    core.subdivide(drape, 1)
    solidify(drape, 0.0025, -1.0)
    drape.data.transform(Matrix.Translation(-neck))
    drape.data.shade_smooth()
    k = kit('nsbx', 'SM_NS_Barber_Cape', ('body',), anchor=(0.0, 0.0, 0.0), dust=0.0, low=0.0)
    TEXTURE_SIZES.update({'nsbx_body': 1024})
    k.add(drape, 'nylon', 0x141418, aux=1, soft=True)
    k.add(tube('collar', [(0.075 * math.cos(a), 0.079 * math.sin(a), 0.012) for a in np.linspace(0, 2 * math.pi, 40, endpoint=False)], 0.009, 8, closed=True),
          'nylon', 0x0c0c0e)
    k.add(bx('snap', 0.07, 0.09, -0.012, 0.012, 0.0, 0.024, 0.004), 'chrome')
    return [k.finish(60.0)]


# ================================================================== build

BUILDERS = {'front': lambda: front(), 'shell': lambda: shell(), 'door': lambda: door() + door_bell(), 'grille': lambda: grille(),
            'pole': lambda: pole(), 'chair': lambda: chair(), 'station': lambda: station(), 'mirror': lambda: mirror(),
            'waiting': lambda: waiting(), 'wall': lambda: wall(), 'lights': lambda: lights(), 'sink': lambda: sink(), 'cape': lambda: cape()}


def build():
    core.reset()
    random.seed(1818)
    UNB.clear()
    PROXIES.clear()
    unbaked('ns_glass')
    ALL.clear()
    for key in WHICH:
        if key in BUILDERS:
            ALL.extend(BUILDERS[key]())
    for o in ALL:
        o.data.name = o.name
    return list(ALL)


def bake_parts(objs):
    return split_for_bake(objs)


def assemble(objs):
    return join_after_bake(ALL, MESHES)


# ================================================================== review

AY = ANCHOR[1]


def L(x, y, z=0.0):
    """Local shop coordinates to Blender (SITE)."""
    return (x, y + AY, z)


# PROP placements in the review (Blender SITE location, stage yaw F): as the docstring tells the game.
PLACE = {
    'SM_NS_Barber_Chair': [(L(CHAIR_X[0], CHAIR_Y), 90.0), (L(CHAIR_X[1], CHAIR_Y), 90.0)],
    'SM_NS_Barber_Door': [(L(DOOR_X, DOOR_Y[0]), 0.0)],
    'SM_NS_DoorBell': [(L(0.72, -1.60, 2.37), 0.0)],
    'SM_NS_Barber_Grille': [(L(GRILLE_X, 0.0, GRILLE_TOP), 0.0)],
    'SM_NS_Barber_Pole': [(L(-0.165, 2.87), 0.0)],
    'SM_NS_Barber_Pole_GlowWarm': [(L(-0.165, 2.87), 0.0)],
    'SM_NS_Barber_Cape': [(L(CHAIR_X[0], CHAIR_Y - 0.125, 1.15), 90.0)],
}
GLOW_REVIEW = {'SM_NS_Barber_Front_GlowNeon': (0xff2a1c, 9.0), 'SM_NS_Barber_Pole_GlowWarm': (0xfff0d8, 6.0),
               'SM_NS_Barber_Lights_GlowWarm': (0xffe2b8, 6.0), 'SM_NS_Barber_Lights_GlowCool': (0xf2f6ff, 7.0)}


def _context(temp, day):
    """A stand-in for what surrounds the shop: the walk-up's brick front with the opening, the sidewalk, the street."""
    brick = rv_mat('rv_brick', 0x6a3326, 0.85)
    iron = rv_mat('rv_iron', 0x1c3a2a, 0.45)
    walk = rv_mat('rv_walk', 0x7d7a73 if day else 0x4a4844, 0.8 if day else 0.3)
    road = rv_mat('rv_road', 0x2a2b2e, 0.7 if day else 0.15)
    sign = rv_mat('rv_sign', 0x0d0f0e, 0.4)
    for (x0, x1, y0, y1, z0, z1, mt) in ((-0.3, 0.0, -9.0, OPEN_Y[0], 0.0, 12.0, brick), (-0.3, 0.0, OPEN_Y[1], 9.0, 0.0, 12.0, brick),
                                          (-0.3, 0.0, OPEN_Y[0], OPEN_Y[1], 3.35, 12.0, brick), (-0.17, 0.0, OPEN_Y[1], OPEN_Y[1] + 0.45, 0.0, 3.35, iron),
                                          (-0.17, 0.0, OPEN_Y[0] - 0.58, OPEN_Y[0], 0.0, 3.35, iron), (-0.12, 0.0, -9.0, 9.0, 3.35, 4.38, sign),
                                          (-4.2, 0.0, -12.0, 12.0, -0.02, 0.0, walk), (-16.2, -4.2, -12.0, 12.0, -0.17, -0.15, road),
                                          (0.0, 9.3, ROOM_Y[0] - 0.3, ROOM_Y[0] - 0.01, 0.0, 3.5, brick), (0.0, 9.3, ROOM_Y[1] + 0.01, ROOM_Y[1] + 0.3, 0.0, 3.5, brick),
                                          (9.08, 9.3, -3.0, 3.0, 0.0, 3.5, brick), (0.0, 9.3, -3.0, 3.0, 3.32, 3.6, brick)):
        o = bx('rv_ctx', x0, x1, y0 + AY, y1 + AY, z0, z1)
        core.assign(o, mt)
        temp.append(o)


def _lights(temp, day, shop_on=True):
    if shop_on:
        for (x0, x1) in ((1.65, 2.85), (3.85, 5.05)):
            temp.append(rv_light('rv_tube', 'AREA', L((x0 + x1) / 2, 1.90, 3.22), 120.0, (0.93, 0.97, 1.0), size=1.2, size_y=0.14,
                                 rot=(0.0, 0.0, 0.0)))
        for x in CHAIR_X:
            temp.append(rv_light('rv_globe', 'POINT', L(x, 0.75, 2.46), 45.0, (1.0, 0.82, 0.6), size=0.1))
        temp.append(rv_light('rv_back', 'AREA', L(7.6, 0.0, 3.2), 70.0, (0.95, 0.97, 1.0), size=1.2, size_y=0.14))
        temp.append(rv_light('rv_neon', 'POINT', L(0.2, -0.40, 1.55), 6.0, (1.0, 0.15, 0.08), size=0.15))
    if day:
        temp.append(rv_light('rv_sun', 'SUN', (0, 0, 10), 4.0, (1.0, 0.96, 0.9), size=2.0, rot=(math.radians(50), 0.0, math.radians(-100))))
        bpy.context.scene.world.color = (0.55, 0.6, 0.68)
    else:
        temp.append(rv_light('rv_lamp', 'SPOT', L(-4.0, -3.0, 7.5), 5500.0, (1.0, 0.62, 0.3), size=0.3, target=L(-1.0, 0.5, 0.0), spot=80))
        temp.append(rv_light('rv_lamp2', 'SPOT', L(-4.0, 9.0, 7.5), 3500.0, (1.0, 0.62, 0.3), size=0.3, target=L(-1.0, 6.0, 0.0), spot=80))
        bpy.context.scene.world.color = (0.004, 0.005, 0.009)


SHEETS = {
    'ns_barber_room': [
        ('street', L(-6.5, -1.2, 1.65), L(0.6, 0.6, 1.55), 26, 'night'),
        ('street_day', L(-7.0, 1.5, 1.7), L(0.6, -0.2, 1.6), 24, 'day'),
        ('inside_front', L(1.2, -2.0, 1.62), L(6.5, 1.6, 1.25), 17, 'night'),
        ('inside_back', L(8.4, -1.8, 1.65), L(1.5, 0.8, 1.45), 17, 'night'),
        ('door', L(-1.6, -2.2, 1.6), L(0.6, -1.6, 1.35), 30, 'night'),
        ('shut', L(-5.5, 1.5, 1.6), L(0.3, -0.3, 1.6), 26, 'shut'),
    ],
    'ns_barber_detail': [
        ('station', L(2.0, -0.4, 1.45), L(3.4, 2.45, 1.05), 24, 'night'),
        ('chair', L(3.5, -0.2, 1.25), L(2.6, 0.95, 0.75), 30, 'night'),
        ('counter', L(2.5, 1.2, 1.35), L(2.6, 2.3, 0.95), 28, 'night'),
        ('waiting', L(3.2, 1.3, 1.5), L(2.6, -2.4, 1.3), 22, 'night'),
        ('back', L(5.6, -0.8, 1.7), L(9.0, 0.4, 1.9), 22, 'night'),
        ('ceiling', L(3.0, -1.6, 1.5), L(6.0, 1.2, 3.3), 20, 'night'),
        ('transom', L(-2.2, 0.9, 1.75), L(0.1, 0.4, 2.75), 30, 'night'),
        ('transom_day', L(-2.4, -0.4, 1.75), L(0.1, 0.4, 2.75), 30, 'day'),
    ],
}


def room_review(objs, which):
    sc = bpy.context.scene
    tmp_dir = os.path.join(core.BUILD_DIR, 'review', 'ns_barber')
    os.makedirs(tmp_dir, exist_ok=True)
    temp = []
    by = {o.name: o for o in objs}
    grilles = []
    for n, places in PLACE.items():
        if n in by:
            for i, (loc, F) in enumerate(places):
                d = instance(by[n], loc, prop_yaw(F))
                temp.append(d)
                if n == 'SM_NS_Barber_Grille':
                    grilles.append(d)
            by[n].hide_render = True
    for n, (hexc, st) in GLOW_REVIEW.items():
        if n in by:
            glow_tint(by[n], hexc, st)
    for o in list(by.values()) + temp:
        if o.name.startswith(tuple(GLOW_REVIEW)):
            o.visible_shadow = False
    cd = bpy.data.cameras.new('rv_cam')
    cam = core.link(bpy.data.objects.new('rv_cam', cd))
    cd.clip_start = 0.02
    cd.clip_end = 200.0
    sc.camera = cam
    temp.append(cam)
    out = {}
    try:
        for mode in ('night', 'day', 'shut'):
            jobs = [(sheet, sh) for sheet, shots in SHEETS.items() if sheet.split('_')[-1] in which for sh in shots if sh[4] == mode]
            if not jobs:
                continue
            ctx = []
            _context(ctx, mode == 'day')
            _lights(ctx, mode == 'day', shop_on=mode != 'shut')
            neon = by.get('SM_NS_Barber_Front_GlowNeon')
            for g in grilles:
                g.scale.z = 1.0 if mode == 'shut' else 0.03
            if mode == 'shut':
                ctx.append(rv_light('rv_night', 'POINT', L(4.5, 0.0, 3.0), 12.0, (1.0, 0.85, 0.65), size=0.1))
                if neon is not None:
                    neon.hide_render = True
            for sheet, (label, loc, tgt, lens, _) in jobs:
                out[(sheet, label)] = rv_shot(cam, os.path.join(tmp_dir, f'{label}.png'), loc, tgt, lens, 960, 640, 128)
            if neon is not None:
                neon.hide_render = False
            for o in ctx:
                bpy.data.objects.remove(o)
        for sheet, shots in SHEETS.items():
            paths = [out[(sheet, sh[0])] for sh in shots if (sheet, sh[0]) in out]
            if paths:
                rv_compose(sheet, paths, 2, LOG)
    finally:
        for o in temp:
            try:
                bpy.data.objects.remove(o)
            except ReferenceError:
                pass
        for n in PLACE:
            if n in by:
                by[n].hide_render = False
        sc.camera = None
        sc.world.color = (0.006, 0.0065, 0.008)


def pose_for_review(objs):
    """Renders ns_barber_room.jpg (and the close-ups), then lines the PROPs up for the standard sheet."""
    global REVIEW_VIEWS
    which = {s.strip() for s in os.environ.get('NSB_REVIEW', 'room,detail').split(',') if s.strip()}
    for p in PROXIES:
        p.hide_render = True
    if which:
        room_review(objs, which)
    props = [o for o in objs if o.name in PLACE]
    if props:
        x = 0.0
        for o in objs:
            o.hide_render = o not in props
        for o in props:
            lo, hi = core.bounds([o])
            o.location = (x - lo.x, -(lo.y + hi.y) / 2, -lo.z)
            x += max(hi.x - lo.x, hi.y - lo.y) + 0.4
        # matrix_world is stale until the view layer updates: without this the sheet frames the props at the origin
        # and the far end of the row (the pole, the chair, the cape) falls out of shot.
        bpy.context.view_layer.update()
        objs[:] = props
    REVIEW_VIEWS = None
